#include "labelprint/transport.h"

#include <windows.h>
#include <winspool.h>
#include <iostream>

namespace labelprint {

void WindowsRawTransport::send(const PrintJob& job, const PrinterConnection& conn) {
    if (conn.name.empty()) {
        throw std::runtime_error("Printer name is required");
    }

    HANDLE hPrinter = nullptr;
    if (!OpenPrinterA(const_cast<char*>(conn.name.c_str()),
                       &hPrinter, nullptr)) {
        throw std::runtime_error("Cannot open printer: " + conn.name);
    }

    DOC_INFO_1A docInfo;
    docInfo.pDocName  = const_cast<char*>("Label Print Job");
    docInfo.pOutputFile = nullptr;
    docInfo.pDatatype  = const_cast<char*>("RAW");

    DWORD jobId = StartDocPrinterA(hPrinter, 1,
                                    reinterpret_cast<LPBYTE>(&docInfo));
    if (jobId == 0) {
        ClosePrinter(hPrinter);
        throw std::runtime_error("StartDocPrinter failed");
    }

    if (!StartPagePrinter(hPrinter)) {
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        throw std::runtime_error("StartPagePrinter failed");
    }

    DWORD written = 0;
    if (!WritePrinter(hPrinter,
                       const_cast<LPBYTE>(job.data.data()),
                       static_cast<DWORD>(job.data.size()),
                       &written)) {
        EndPagePrinter(hPrinter);
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        throw std::runtime_error("WritePrinter failed");
    }

    EndPagePrinter(hPrinter);
    EndDocPrinter(hPrinter);
    ClosePrinter(hPrinter);

    std::cout << "// WindowsRawTransport: " << written
              << " bytes -> " << conn.name << std::endl;
}

} // namespace labelprint
