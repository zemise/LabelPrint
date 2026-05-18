param(
    [string]$OutputFile = "label_medical_cn.prn"
)

Add-Type -AssemblyName System.Drawing

# Generates the confirmed-working Chinese TSPL print payload for Xprinter XP-360B.
# Chinese text is rasterized into BITMAP commands because the printer's built-in
# text path was not reliable for CJK output in this project.

function From-CodePoints {
    param(
        [int[]]$CodePoints
    )

    $builder = New-Object System.Text.StringBuilder
    foreach ($cp in $CodePoints) {
        [void]$builder.Append([char]$cp)
    }
    return $builder.ToString()
}

function Get-PreferredFontName {
    $candidates = @("SimHei", "Microsoft YaHei", "SimSun")
    foreach ($name in $candidates) {
        try {
            $font = New-Object System.Drawing.Font($name, 12)
            # Font.Name may return a localized name; the font exists if no exception was thrown.
            $actual = $font.Name
            $font.Dispose()
            Write-Host "Found Chinese font: $name (system name: $actual)"
            return $name
        }
        catch {
        }
    }
    throw "No suitable Chinese font found."
}

function New-TextBitmapData {
    param(
        [int]$Width,
        [int]$Height,
        [string]$Text,
        [string]$FontName,
        [float]$FontSize,
        [bool]$BlackIsOne
    )

    $bitmap = New-Object System.Drawing.Bitmap $Width, $Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::White)
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
    $font = New-Object System.Drawing.Font($FontName, $FontSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.DrawString($Text, $font, [System.Drawing.Brushes]::Black, ([System.Drawing.PointF]::new([single]0, [single]0)))

    $widthBytes = [int][Math]::Ceiling($bitmap.Width / 8.0)
    $bytes = New-Object byte[] ($widthBytes * $bitmap.Height)
    $index = 0

    # Background bit: 1 = white-background-is-1, 0 = black-is-1
    $bgBit = if ($BlackIsOne) { 0 } else { 1 }

    for ($y = 0; $y -lt $bitmap.Height; $y++) {
        for ($xb = 0; $xb -lt $widthBytes; $xb++) {
            $byte = 0
            for ($bit = 0; $bit -lt 8; $bit++) {
                $x = $xb * 8 + $bit
                if ($x -lt $bitmap.Width) {
                    $pixel = $bitmap.GetPixel($x, $y)
                    $isBlack = (($pixel.R + $pixel.G + $pixel.B) / 3) -lt 180
                    if (($BlackIsOne -and $isBlack) -or ((-not $BlackIsOne) -and (-not $isBlack))) {
                        $byte = $byte -bor (1 -shl (7 - $bit))
                    }
                } else {
                    # Padding beyond bitmap width: fill with background color
                    if ($bgBit) {
                        $byte = $byte -bor (1 -shl (7 - $bit))
                    }
                }
            }
            $bytes[$index] = [byte]$byte
            $index++
        }
    }

    $graphics.Dispose()
    $font.Dispose()
    $bitmap.Dispose()

    return @{
        WidthBytes = $widthBytes
        Height = $Height
        Bytes = $bytes
    }
}

function Write-AsciiLine {
    param(
        [System.IO.BinaryWriter]$Writer,
        [string]$Line
    )

    $bytes = [System.Text.Encoding]::ASCII.GetBytes($Line + "`r`n")
    $Writer.Write($bytes)
}

function Write-BitmapCommand {
    param(
        [System.IO.BinaryWriter]$Writer,
        [int]$X,
        [int]$Y,
        [hashtable]$BitmapData
    )

    $header = "BITMAP $X,$Y,$($BitmapData.WidthBytes),$($BitmapData.Height),0,"
    $Writer.Write([System.Text.Encoding]::ASCII.GetBytes($header))
    $Writer.Write($BitmapData.Bytes)
    $Writer.Write([System.Text.Encoding]::ASCII.GetBytes("`r`n"))
}

$fontName = Get-PreferredFontName

# Unicode literals are built from code points so this script stays ASCII-safe.
$itemText = From-CodePoints @(0x8840, 0x5E38, 0x89C4, 0xFF08, 0x8FC8, 0x745E, 0x6D41, 0x6C34, 0x7EBF, 0xFF09)
$nameText = From-CodePoints @(0x5ED6, 0x660E)
$specimenText = From-CodePoints @(0x5168, 0x8840)
$deptText = From-CodePoints @(0x5FC3, 0x8840, 0x7BA1, 0x5185, 0x79D1, 0x4E8C, 0x533A)

# The printer was verified to behave correctly with the same polarity as the
# former diagnostic "B" sample: white background bits set to 1, black text bits set to 0.
$itemBmp = New-TextBitmapData -Width 250 -Height 30 -Text $itemText -FontName $fontName -FontSize 22 -BlackIsOne $false
$nameBmp = New-TextBitmapData -Width 90 -Height 28 -Text $nameText -FontName $fontName -FontSize 24 -BlackIsOne $false
$specimenBmp = New-TextBitmapData -Width 80 -Height 28 -Text $specimenText -FontName $fontName -FontSize 24 -BlackIsOne $false
$deptBmp = New-TextBitmapData -Width 150 -Height 26 -Text $deptText -FontName $fontName -FontSize 19 -BlackIsOne $false

$path = Join-Path $PSScriptRoot $OutputFile
$stream = [System.IO.File]::Open($path, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
$writer = New-Object System.IO.BinaryWriter($stream)

try {
    Write-AsciiLine -Writer $writer -Line "SIZE 50 mm,30 mm"
    Write-AsciiLine -Writer $writer -Line "GAP 2 mm,0 mm"
    Write-AsciiLine -Writer $writer -Line "DENSITY 12"
    Write-AsciiLine -Writer $writer -Line "SPEED 4"
    Write-AsciiLine -Writer $writer -Line "DIRECTION 1"
    Write-AsciiLine -Writer $writer -Line "REFERENCE 5,5"
    Write-AsciiLine -Writer $writer -Line "CLS"
    Write-AsciiLine -Writer $writer -Line 'TEXT 5,5,"3",0,1,2,"22"'
    Write-BitmapCommand -Writer $writer -X 5 -Y 44 -BitmapData $itemBmp
    Write-AsciiLine -Writer $writer -Line 'BARCODE 20,72,"128",75,0,0,3,9,"008085125"'
    Write-AsciiLine -Writer $writer -Line 'TEXT 135,152,"3",0,1,1,"008085125"'
    Write-BitmapCommand -Writer $writer -X 5 -Y 175 -BitmapData $nameBmp
    Write-BitmapCommand -Writer $writer -X 145 -Y 175 -BitmapData $specimenBmp
    Write-BitmapCommand -Writer $writer -X 245 -Y 175 -BitmapData $deptBmp
    Write-AsciiLine -Writer $writer -Line 'TEXT 5,205,"3",0,1,1,"202629988"'
    Write-AsciiLine -Writer $writer -Line 'TEXT 245,205,"3",0,1,1,"2026/5/15 9:24"'
    Write-AsciiLine -Writer $writer -Line "PRINT 1,1"
}
finally {
    $writer.Dispose()
    $stream.Dispose()
}

Write-Host "TSPL binary generated -> $path"
Write-Host "Chinese font -> $fontName"
