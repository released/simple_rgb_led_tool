@echo off
setlocal
pushd "%~dp0\.."

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$proc = Get-Process -Name 'RgbLedTool' -ErrorAction SilentlyContinue; if ($proc) { $proc | Stop-Process -Force }"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_mfc.ps1" -Configuration Release -Platform x64
if errorlevel 1 (
  popd
  exit /b %errorlevel%
)

echo Build complete: build\RgbLedTool.exe
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$exe = Resolve-Path '%~dp0..\\build\\RgbLedTool.exe' -ErrorAction SilentlyContinue; if ($exe) { Start-Process -FilePath $exe }"

popd
endlocal
