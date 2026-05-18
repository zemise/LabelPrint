#include "labelprint/transport.h"
#include <fstream>
#include <iostream>

namespace labelprint {

void FileTransport::send(const PrintJob& job, const PrinterConnection& conn) {
    std::string filename = conn.name;
    if (filename.empty()) {
        filename = "label_output." + job.format;
    }

    std::ofstream f(filename, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    f.write(reinterpret_cast<const char*>(job.data.data()), job.data.size());
    std::cout << "// FileTransport: " << job.data.size()
              << " bytes -> " << filename << std::endl;
}

} // namespace labelprint
