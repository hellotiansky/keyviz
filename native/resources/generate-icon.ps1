$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

function New-IconFrame([int]$Size) {
    $bitmap = New-Object System.Drawing.Bitmap($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.Clear([System.Drawing.Color]::Transparent
        )
        $scale = $Size / 256.0
        $graphics.ScaleTransform([single]$scale, [single]$scale)

        $background = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 20, 30, 48))
        $tile = New-Object System.Drawing.Drawing2D.GraphicsPath
        $tile.AddArc(16, 16, 48, 48, 180, 90)
        $tile.AddArc(192, 16, 48, 48, 270, 90)
        $tile.AddArc(192, 192, 48, 48, 0, 90)
        $tile.AddArc(16, 192, 48, 48, 90, 90)
        $tile.CloseFigure()
        $graphics.FillPath($background, $tile)

        $keyBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 61, 220, 190))
        $keyPath = New-Object System.Drawing.Drawing2D.GraphicsPath
        $keyPath.AddArc(62, 58, 36, 36, 180, 90)
        $keyPath.AddArc(158, 58, 36, 36, 270, 90)
        $keyPath.AddArc(158, 154, 36, 36, 0, 90)
        $keyPath.AddArc(62, 154, 36, 36, 90, 90)
        $keyPath.CloseFigure()
        $graphics.FillPath($keyBrush, $keyPath)

        $smallBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 235, 246, 255))
        $graphics.FillEllipse($smallBrush, 116, 104, 24, 24)
        $graphics.FillEllipse($smallBrush, 116, 146, 24, 24)
        $graphics.FillEllipse($smallBrush, 88, 125, 18, 18)
        $graphics.FillEllipse($smallBrush, 150, 125, 18, 18)

        $stream = New-Object System.IO.MemoryStream
        $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
        return ,$stream.ToArray()
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$sizes = @(16, 32, 48, 256)
$frames = @()
foreach ($size in $sizes) {
    $frames += ,(New-IconFrame $size)
}

$output = Join-Path $PSScriptRoot "keyviz.ico"
$stream = [System.IO.File]::Open($output, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
$writer = New-Object System.IO.BinaryWriter($stream)
try {
    $writer.Write([UInt16]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]$frames.Count)
    $offset = 6 + (16 * $frames.Count)
    for ($i = 0; $i -lt $frames.Count; $i++) {
        $size = $sizes[$i]
        $dimension = if ($size -eq 256) { [byte]0 } else { [byte]$size }
        $writer.Write($dimension)
        $writer.Write($dimension)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]32)
        $writer.Write([UInt32]$frames[$i].Length)
        $writer.Write([UInt32]$offset)
        $offset += $frames[$i].Length
    }
    foreach ($frame in $frames) {
        $writer.Write([byte[]]$frame)
    }
}
finally {
    $writer.Dispose()
    $stream.Dispose()
}
Write-Host "Generated $output"
