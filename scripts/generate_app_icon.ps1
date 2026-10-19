param(
    [string]$OutPath = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($OutPath)) {
    $OutPath = Join-Path (Join-Path $PSScriptRoot "..") "src\res\p_tool.ico"
}

Add-Type -AssemblyName System.Drawing

function New-RgbIconBitmap([int]$Size) {
    $bmp = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

    $red = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 255, 186, 186))
    $green = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 188, 238, 196))
    $blue = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 188, 210, 255))
    $black = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::Black)

    $w0 = [Math]::Floor($Size / 3.0)
    $w1 = [Math]::Floor($Size / 3.0)
    $w2 = $Size - $w0 - $w1
    $g.FillRectangle($red, 0, 0, $w0, $Size)
    $g.FillRectangle($green, $w0, 0, $w1, $Size)
    $g.FillRectangle($blue, $w0 + $w1, 0, $w2, $Size)

    $fontSize = [Math]::Max(8.0, $Size * 0.74)
    $font = $null
    foreach ($fontName in @("Segoe UI Black", "Arial Black", "Segoe UI")) {
        try {
            $font = New-Object System.Drawing.Font($fontName, $fontSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
            break
        } catch {
            $font = $null
        }
    }
    if ($null -eq $font) {
        $font = New-Object System.Drawing.Font("Arial", $fontSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    }

    $fmt = New-Object System.Drawing.StringFormat
    $fmt.Alignment = [System.Drawing.StringAlignment]::Center
    $fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
    $textRect = New-Object System.Drawing.RectangleF(0, [float](-$Size * 0.03), $Size, $Size)
    $g.DrawString("R", $font, $black, $textRect, $fmt)

    $fmt.Dispose()
    $font.Dispose()
    $black.Dispose()
    $red.Dispose()
    $green.Dispose()
    $blue.Dispose()
    $g.Dispose()

    return $bmp
}

function Convert-BitmapToIcoDibBytes([System.Drawing.Bitmap]$Bitmap) {
    $size = $Bitmap.Width
    $xorStride = $size * 4
    $andStride = [Math]::Floor((($size + 31) / 32)) * 4
    $xorBytes = $xorStride * $size
    $andBytes = $andStride * $size
    $stream = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter $stream

    $bw.Write([UInt32]40)
    $bw.Write([Int32]$size)
    $bw.Write([Int32]($size * 2))
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]0)
    $bw.Write([UInt32]($xorBytes + $andBytes))
    $bw.Write([Int32]0)
    $bw.Write([Int32]0)
    $bw.Write([UInt32]0)
    $bw.Write([UInt32]0)

    for ($y = $size - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $size; $x++) {
            $c = $Bitmap.GetPixel($x, $y)
            $bw.Write([Byte]$c.B)
            $bw.Write([Byte]$c.G)
            $bw.Write([Byte]$c.R)
            $bw.Write([Byte]$c.A)
        }
    }

    for ($i = 0; $i -lt $andBytes; $i++) {
        $bw.Write([Byte]0)
    }

    $bytes = $stream.ToArray()
    $bw.Close()
    $stream.Dispose()
    return $bytes
}

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$entries = @()
foreach ($size in $sizes) {
    $bitmap = New-RgbIconBitmap $size
    $entries += ,@{
        Size = $size
        Bytes = Convert-BitmapToIcoDibBytes $bitmap
        Offset = 0
    }
    $bitmap.Dispose()
}

$dirSize = 6 + ($entries.Count * 16)
$offset = $dirSize
foreach ($entry in $entries) {
    $entry.Offset = $offset
    $offset += $entry.Bytes.Length
}

$fs = [System.IO.File]::Open($OutPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
$bw = New-Object System.IO.BinaryWriter $fs

$bw.Write([UInt16]0)
$bw.Write([UInt16]1)
$bw.Write([UInt16]$entries.Count)

foreach ($entry in $entries) {
    $sizeByte = if ($entry.Size -eq 256) { 0 } else { $entry.Size }
    $bw.Write([Byte]$sizeByte)
    $bw.Write([Byte]$sizeByte)
    $bw.Write([Byte]0)
    $bw.Write([Byte]0)
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]$entry.Bytes.Length)
    $bw.Write([UInt32]$entry.Offset)
}

foreach ($entry in $entries) {
    $bw.Write([Byte[]]$entry.Bytes)
}

$bw.Close()
$fs.Close()

Write-Output "Generated icon: $OutPath"
