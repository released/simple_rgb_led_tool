param(
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [string]$HubRoot = 'D:/SourceCode/python/#READ_ME_FIRST#'
)
$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $HubRoot 'scripts/invoke_hub_test.ps1')
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation -or $LASTEXITCODE -ne 0) { throw 'Visual Studio C++ tools are required.' }
$vc = (Get-ChildItem -LiteralPath (Join-Path $installation 'VC/Tools/MSVC') -Directory |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1).FullName
$sdk = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'
$sdkVersion = (Get-ChildItem -LiteralPath (Join-Path $sdk 'Include') -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um/Windows.h') } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1).Name
$savedInclude = $env:INCLUDE
$savedLib = $env:LIB
$savedPath = $env:PATH
$env:INCLUDE = "$vc/include;$vc/atlmfc/include;$sdk/Include/$sdkVersion/ucrt;$sdk/Include/$sdkVersion/shared;$sdk/Include/$sdkVersion/um"
$env:LIB = "$vc/lib/x64;$vc/atlmfc/lib/x64;$sdk/Lib/$sdkVersion/ucrt/x64;$sdk/Lib/$sdkVersion/um/x64"
$env:PATH = "$vc/bin/Hostx64/x64;" + $env:PATH
$compiler = Join-Path $vc 'bin/Hostx64/x64/cl.exe'
$linker = Join-Path $vc 'bin/Hostx64/x64/link.exe'
$ownedProcesses = New-Object 'Collections.Generic.List[Diagnostics.Process]'
$nativeEvidence = New-Object 'Collections.Generic.List[object]'
$flags = @('/nologo','/EHsc','/std:c++17','/W4','/WX','/utf-8',('/I'+$project),
    '/D_WINDOWS','/D_UNICODE','/DUNICODE','/D_AFX_NO_MFC_CONTROLS_IN_DIALOGS')
$flags += if ($Configuration -eq 'Debug') { @('/MTd','/D_DEBUG','/Od') } else { @('/MT','/DNDEBUG','/O2') }
$sources = @('src/core/rgb_pattern.cpp','src/core/rgb_pattern_codegen.cpp',
    'src/core/rgb_pattern_file.cpp','src/core/text_utils.cpp')
function Invoke-OwnedNative($Run, [string]$Executable, [string[]]$Arguments, [string]$Label) {
    $stdout = Join-Path $Run.Work ($Label+'.stdout.txt')
    $stderr = Join-Path $Run.Work ($Label+'.stderr.txt')
    # All arguments are fixed flags or owned absolute paths, quoted as Windows arguments.
    $quoted = @($Arguments | ForEach-Object { '"'+($_ -replace '(\\*)"','$1$1\"' -replace '(\\+)$','$1$1')+'"' })
    $process = Start-Process -FilePath $Executable -ArgumentList $quoted -WorkingDirectory $Run.Work -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    $null = $process.Handle
    $ownedProcesses.Add($process)
    try { if (!$process.HasExited) { Register-HubTestProcess -Run $Run -ProcessId $process.Id } }
    catch { if (!$process.HasExited) { throw } }
    if (!$process.WaitForExit(45000)) { throw ($Label+' timed out.') }
    $process.WaitForExit()
    $lines = @([IO.File]::ReadAllLines($stdout)) + @([IO.File]::ReadAllLines($stderr))
    $nativeEvidence.Add(@{stage=$Label; exit_code=$process.ExitCode; output=$lines})
    $lines | Write-Output
    if ($process.ExitCode -ne 0) { throw ($Label+' failed: exit '+$process.ExitCode) }
}
try {
    $result = Invoke-HubOwnedTest -ProjectRoot $project -Purpose ("pattern contract $Configuration; deterministic core/native UI; no device I/O") -MaxBytes 40MB -Action {
        param($run)
        $fixtures = Join-Path $run.Work 'fixtures'
        [void][IO.Directory]::CreateDirectory($fixtures)
        $coreSources = @($sources | ForEach-Object { Join-Path $project $_ })
        Invoke-OwnedNative $run $compiler ($flags + @('/Fe:core-contract.exe',(Join-Path $PSScriptRoot 'tests/pattern_contract.cpp')) + $coreSources) 'compile-core'
        Invoke-OwnedNative $run (Join-Path $run.Work 'core-contract.exe') @($fixtures) 'core'
        $uiSources = @('src/core/app_state.cpp','src/config/ini_manager.cpp','src/ui/rgb_led_tab.cpp') |
            ForEach-Object { Join-Path $project $_ }
        Invoke-OwnedNative $run $compiler ($flags + @('/c',(Join-Path $PSScriptRoot 'tests/pattern_ui_contract.cpp')) + $uiSources) 'compile-ui'
        Invoke-OwnedNative $run $linker @('/nologo','/SUBSYSTEM:CONSOLE','/ENTRY:wmainCRTStartup','/OUT:ui-contract.exe',
            'pattern_ui_contract.obj','rgb_led_tab.obj','rgb_pattern.obj','rgb_pattern_codegen.obj','rgb_pattern_file.obj',
            'text_utils.obj','app_state.obj','ini_manager.obj') 'link-ui'
        Invoke-OwnedNative $run (Join-Path $run.Work 'ui-contract.exe') @($fixtures) 'ui'
    } -Teardown {
        param($run)
        foreach ($process in $ownedProcesses) {
            if (!$process.HasExited) { $process.Kill(); $process.WaitForExit() }
            $process.Dispose()
        }
    } -CollectArtifacts {
        param($run)
        Register-HubArtifactTree -Run $run -Producer 'fresh exclusive pattern contract compiler/harness output'
    }
    $evidence = [ordered]@{configuration=$Configuration; outcome=$result.Outcome;
        cleanup_removed=$result.CleanupRemoved; run_root=$result.RunRoot;
        compiler=$compiler; sdk=$sdkVersion; checks=@($result.Output | Where-Object { $_ -match '^PASS:' });
        errors=$result.Errors; native=@($nativeEvidence.ToArray()); sources=@()}
    foreach ($relative in @($sources + @('src/ui/rgb_led_tab.cpp','src/services/rgb_hid_protocol.h',
        'scripts/tests/pattern_contract.cpp','scripts/tests/pattern_ui_contract.cpp','scripts/verify_pattern_contract.ps1'))) {
        $evidence.sources += @{path=$relative; sha256=(Get-FileHash -LiteralPath (Join-Path $project $relative)).Hash}
    }
    $directory = Join-Path $project 'docs/internal'
    [void][IO.Directory]::CreateDirectory($directory)
    [IO.File]::WriteAllText((Join-Path $directory ("pattern-contract-$Configuration.json")),
        ($evidence | ConvertTo-Json -Depth 7), (New-Object Text.UTF8Encoding($false)))
    $result.Output | Write-Output
    if (!$result.Success) { $result.Errors | Format-List; throw 'Pattern contract verification or cleanup failed.' }
    Write-Output ("PASS: $Configuration contract checks; registered outputs retired.")
} finally {
    $env:INCLUDE = $savedInclude
    $env:LIB = $savedLib
    $env:PATH = $savedPath
}
