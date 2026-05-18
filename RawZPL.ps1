# RawZPL.ps1 — 绕过驱动直接发送 ZPL 到打印机
param(
    [string]$PrinterName = "ZDesigner ZD888-203dpi ZPL",
    [string]$ZplFile     = "label_medical.zpl"
)

$zpl = [IO.File]::ReadAllText((Join-Path $PSScriptRoot $ZplFile))

$code = @'
using System;
using System.IO;
using System.Runtime.InteropServices;

public class RawPrinter {
    [StructLayout(LayoutKind.Sequential)]
    private struct DOCINFOA {
        [MarshalAs(UnmanagedType.LPStr)] public string pDocName;
        [MarshalAs(UnmanagedType.LPStr)] public string pOutputFile;
        [MarshalAs(UnmanagedType.LPStr)] public string pDataType;
    }

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int OpenPrinter(string pPrinterName, out IntPtr hPrinter, IntPtr pDefault);

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int StartDocPrinter(IntPtr hPrinter, int level, ref DOCINFOA di);

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int EndDocPrinter(IntPtr hPrinter);

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int StartPagePrinter(IntPtr hPrinter);

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int EndPagePrinter(IntPtr hPrinter);

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int WritePrinter(IntPtr hPrinter, byte[] pBuf, int cbBuf, out int pcWritten);

    [DllImport("winspool.Drv", SetLastError = true)]
    private static extern int ClosePrinter(IntPtr hPrinter);

    public static void SendRaw(string printer, byte[] data) {
        IntPtr hPrinter = IntPtr.Zero;
        if (OpenPrinter(printer, out hPrinter, IntPtr.Zero) == 0)
            throw new Exception("Cannot open printer: " + printer);

        var di = new DOCINFOA { pDocName = "ZPL Label", pDataType = "RAW" };
        StartDocPrinter(hPrinter, 1, ref di);
        StartPagePrinter(hPrinter);

        int written;
        WritePrinter(hPrinter, data, data.Length, out written);

        EndPagePrinter(hPrinter);
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
    }
}
'@

Add-Type -TypeDefinition $code -ErrorAction Stop
[RawPrinter]::SendRaw($PrinterName, [Text.Encoding]::UTF8.GetBytes($zpl))
Write-Host "sent $($zpl.Length) bytes → $PrinterName"
