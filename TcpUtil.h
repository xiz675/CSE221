#pragma once
#include "rely.h"
#include <chrono>

#if defined(_WIN32)
#   define SOCKET_TYPE uintptr_t
#else 
#   define SOCKET_TYPE int32_t
#endif

class TcpConnection
{
    friend class TcpServer;
private:
    std::string Address;
    // client socket descriptor
    SOCKET_TYPE ClientSocket;
    uint16_t Port;
    bool IsClosed = false;
    TcpConnection(SOCKET_TYPE socket, const std::string& ip, const uint16_t port);
public:
    const std::chrono::time_point<std::chrono::high_resolution_clock> Timestamp;
    TcpConnection(const std::string& ip, const uint16_t port);
    TcpConnection(TcpConnection&& conn);
    TcpConnection(const TcpConnection& conn) = delete;
    ~TcpConnection();
    TcpConnection& operator=(const TcpConnection& conn) = delete;
    TcpConnection& operator=(TcpConnection&& conn) = delete;
    bool SendData(const void* data, const size_t size);
    bool ReceiveData(void* data, const size_t size);
    //may not close connection immediately
    void Close();
};

class TcpServer
{
public:
private:
    // server socket descriptor
    SOCKET_TYPE ServerSocket;
    const uint16_t Port;
public:
    TcpServer(const uint16_t port);
    ~TcpServer();
    TcpConnection WaitConnection() const;
};


