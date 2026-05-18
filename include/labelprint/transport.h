#pragma once

#include "labelprint/backend.h"
#include <string>
#include <cstdint>
#include <stdexcept>

namespace labelprint {

// ---------------------------------------------------------------------------
// PrinterConnection — identifies a target printer
// ---------------------------------------------------------------------------
struct PrinterConnection {
    std::string name;        // printer share name (Windows) or IP address
    int         port = 9100; // TCP port for network printers
};

// ---------------------------------------------------------------------------
// IPrintTransport — sends a rendered PrintJob to a printer or file
// ---------------------------------------------------------------------------
class IPrintTransport {
public:
    virtual ~IPrintTransport() = default;
    virtual void send(const PrintJob& job,
                      const PrinterConnection& conn) = 0;
};

// ---------------------------------------------------------------------------
// FileTransport — write job data to a file (for debugging and export)
// ---------------------------------------------------------------------------
class FileTransport : public IPrintTransport {
public:
    void send(const PrintJob& job,
              const PrinterConnection& conn) override;
};

// ---------------------------------------------------------------------------
// WindowsRawTransport — send RAW print job via WinSpool
// ---------------------------------------------------------------------------
class WindowsRawTransport : public IPrintTransport {
public:
    void send(const PrintJob& job,
              const PrinterConnection& conn) override;
};

// ---------------------------------------------------------------------------
// Tcp9100Transport — send job to network printer via TCP port 9100
// ---------------------------------------------------------------------------
class Tcp9100Transport : public IPrintTransport {
public:
    void send(const PrintJob& job,
              const PrinterConnection& conn) override;
};

} // namespace labelprint
