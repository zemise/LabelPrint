#include "labelprint/transport.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace labelprint {

void Tcp9100Transport::send(const PrintJob& job, const PrinterConnection& conn) {
    if (conn.name.empty()) {
        throw std::runtime_error("Printer IP address is required");
    }

    // One-time WinSock init
    static bool wsaReady = [] {
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }();

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Cannot create socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(conn.port));

    if (inet_pton(AF_INET, conn.name.c_str(), &addr.sin_addr) != 1) {
        // Try hostname resolution
        addrinfo hints{};
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* result = nullptr;
        if (getaddrinfo(conn.name.c_str(), nullptr, &hints, &result) != 0 || !result) {
            closesocket(sock);
            throw std::runtime_error("Cannot resolve printer address: " + conn.name);
        }
        addr.sin_addr = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr;
        freeaddrinfo(result);
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        throw std::runtime_error("Cannot connect to printer: " + conn.name +
                                 ":" + std::to_string(conn.port));
    }

    int totalSent = 0;
    const char* p   = reinterpret_cast<const char*>(job.data.data());
    int remaining   = static_cast<int>(job.data.size());

    while (remaining > 0) {
        int n = ::send(sock, p, remaining, 0);
        if (n == SOCKET_ERROR) {
            closesocket(sock);
            throw std::runtime_error("send() failed to " + conn.name);
        }
        totalSent += n;
        p         += n;
        remaining -= n;
    }

    closesocket(sock);
    std::cout << "// Tcp9100Transport: " << totalSent
              << " bytes -> " << conn.name << ":" << conn.port << std::endl;
}

} // namespace labelprint
