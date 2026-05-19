param(
    [string]$PrinterName = "ZDesigner ZD888-203dpi ZPL",
    [string]$ZplFile     = "label_medical.zpl"
)

$path = Join-Path $PSScriptRoot $ZplFile
if (-not (Test-Path $path)) {
    throw "Cannot find ZPL file: $path"
}

$bytes = [IO.File]::ReadAllBytes($path)

$code = @'
using System;
using System.Runtime.InteropServices;

public class RawPrinter {
    [StructLayout(LayoutKind.Sequential)]
    public class DOCINFOA {
        [MarshalAs(UnmanagedType.LPStr)] public string pDocName;
        [MarshalAs(UnmanagedType.LPStr)] public string pOutputFile;
        [MarshalAs(UnmanagedType.LPStr)] public string pDataType;
    }

    [DllImport("winspool.Drv", EntryPoint="OpenPrinterA", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern bool OpenPrinter(string pPrinterName, out IntPtr hPrinter, IntPtr pDefault);

    [DllImport("winspool.Drv", SetLastError=true)]
    public static extern bool StartDocPrinter(IntPtr hPrinter, int level, DOCINFOA di);

    [DllImport("winspool.Drv", SetLastError=true)]
    public static extern bool EndDocPrinter(IntPtr hPrinter);

    [DllImport("winspool.Drv", SetLastError=true)]
    public static extern bool StartPagePrinter(IntPtr hPrinter);

    [DllImport("winspool.Drv", SetLastError=true)]
    public static extern bool EndPagePrinter(IntPtr hPrinter);

    [DllImport("winspool.Drv", SetLastError=true)]
    public static extern bool WritePrinter(IntPtr hPrinter, byte[] pBuf, int cbBuf, out int pcWritten);

    [DllImport("winspool.Drv", SetLastError=true)]
    public static extern bool ClosePrinter(IntPtr hPrinter);
}
'@

Add-Type -TypeDefinition $code -ErrorAction Stop
$doc = New-Object RawPrinter+DOCINFOA
$doc.pDocName = "ZPL Label"
$doc.pDataType = "RAW"

$hPrinter = [IntPtr]::Zero
if (-not [RawPrinter]::OpenPrinter($PrinterName, [ref]$hPrinter, [IntPtr]::Zero)) {
    throw "Cannot open printer: $PrinterName"
}

try {
    if (-not [RawPrinter]::StartDocPrinter($hPrinter, 1, $doc)) {
        throw "StartDocPrinter failed"
    }
    if (-not [RawPrinter]::StartPagePrinter($hPrinter)) {
        throw "StartPagePrinter failed"
    }

    $written = 0
    if (-not [RawPrinter]::WritePrinter($hPrinter, $bytes, $bytes.Length, [ref]$written)) {
        throw "WritePrinter failed"
    }

    [RawPrinter]::EndPagePrinter($hPrinter) | Out-Null
    [RawPrinter]::EndDocPrinter($hPrinter) | Out-Null
    Write-Host "sent $written bytes -> $PrinterName"
}
finally {
    if ($hPrinter -ne [IntPtr]::Zero) {
        [RawPrinter]::ClosePrinter($hPrinter) | Out-Null
    }
}
