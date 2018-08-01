#include "rely.h"
#include "TcpUtil.h"
#include <algorithm>
#if defined(_WIN32)
#   include <winsock2.h> 
#   include <Ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <arpa/inet.h>
#endif


using std::string;

static bool CheckSocket(const SOCKET_TYPE socket)
{
#if defined(_WIN32)
    return socket != INVALID_SOCKET;
#else
    return socket >= 0;
#endif
}

std::string GetNetError()
{
#if defined(_WIN32)
    return std::to_string(WSAGetLastError());
#else
    return std::string(strerror(errno));
#endif
}

#if defined(_WIN32)
struct WSInit
{
    WSInit()
    {
        WSADATA wsaData;
        const auto ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (ret == 0)
            logger::Success("Winsock initializing succeed.\n");
        else
            EXIT_MSG("Winsock initializing failed: %d\n", ret);
    }
};
static void WSInitilize()
{
    static WSInit init;
}
#endif



TcpServer::TcpServer(const uint16_t port) : Port(port)
{
#if defined(_WIN32)
    WSInitilize();
#endif
    ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!CheckSocket(ServerSocket))
        EXIT_MSG("socket failed [%s]\n", GetNetError());
#if defined(__APPLE__)
    const int flag = 1;
    if (setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag)) != 0)
        EXIT_MSG("set reuse failed [%s]\n", GetNetError());
#endif
    // server address
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(Port);
    if (bind(ServerSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) != 0)
        EXIT_MSG("bind failed [%s]\n", GetNetError());
    if (listen(ServerSocket, 8) != 0)
        EXIT_MSG("listen failed [%s]\n", GetNetError());

}

TcpServer::~TcpServer()
{
#if defined(_WIN32)
    closesocket(ServerSocket);
#else
    close(ServerSocket);
#endif
    logger::Info("Server closed\n");
}

TcpConnection TcpServer::WaitConnection() const
{
    while (true)
    {
        struct sockaddr_in clientAddress;
        socklen_t addrLen = (socklen_t)sizeof(clientAddress);
        // wait for a client
        const auto clientSocket = accept(ServerSocket, reinterpret_cast<sockaddr*>(&clientAddress), &addrLen);
        if (!CheckSocket(clientSocket))
        {
            if (errno != EAGAIN)
                logger::Error("accept failed [%s]\n\n", GetNetError());
            continue;
        }
        char addrtmp[INET_ADDRSTRLEN] = { '\0' };
        inet_ntop(AF_INET, &clientAddress.sin_addr, addrtmp, INET_ADDRSTRLEN);
        TcpConnection conn(clientSocket, addrtmp, ntohs(clientAddress.sin_port));
        // logger::Info("Incoming TCP conenction [%s:%u]\n", conn.Address, conn.Port);
        return conn;
    }
}

TcpConnection::TcpConnection(SOCKET_TYPE socket, const std::string& ip, const uint16_t port) : Address(ip), ClientSocket(socket), Port(port), Timestamp(std::chrono::high_resolution_clock::now())
{
}

TcpConnection::TcpConnection(const std::string& ip, const uint16_t port) : Address(ip), Port(port), Timestamp(std::chrono::high_resolution_clock::now())
{
#if defined(_WIN32)
    WSInitilize();
#endif
    ClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (!CheckSocket(ClientSocket))
    {
        logger::Error("build socket failed [%s]\n\n", GetNetError());
        throw std::runtime_error("establish conenction error");
    }
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, Address.c_str(), &serverAddress.sin_addr);
    serverAddress.sin_port = htons(Port);
    const auto ret = connect(ClientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
    if (ret == 0)
        ;// logger::Debug("Outcoming TCP conenction [%s:%u]\n", Address, Port);
    else
    {
        logger::Error("connect to dest failed [%s]\n\n", GetNetError());
        throw std::runtime_error("establish conenction error");
    }
}

TcpConnection::TcpConnection(TcpConnection && conn) : Address(conn.Address), ClientSocket(conn.ClientSocket), Port(conn.Port), IsClosed(conn.IsClosed), Timestamp(conn.Timestamp)
{
    conn.IsClosed = true;
}

TcpConnection::~TcpConnection()
{
    if (!IsClosed)
        Close();
}

bool TcpConnection::SendData(const void * data, const size_t size)
{
    auto* ptr = reinterpret_cast<const uint8_t*>(data);
    for (size_t left = size; left > 0;)
    {
        const auto sendCnt = send(ClientSocket, (const char*)ptr, (int)std::min(left, (size_t)INT32_MAX), 0);
        if (sendCnt == -1)
        {
            logger::Error("Send to [%s:%u] failed [%s].\n", Address, Port, GetNetError());
            return false;
        }
        ptr += sendCnt, left -= sendCnt;
    }
    return true;
}

bool TcpConnection::ReceiveData(void * data, const size_t size)
{
    uint8_t * ptr = reinterpret_cast<uint8_t*>(data);
    for (size_t left = size; left > 0;)
    {
        const auto recvCnt = recv(ClientSocket, (char*)ptr, (int)std::min(left, (size_t)INT32_MAX), 0);
        switch (recvCnt)
        {
        case 0:
            Close();
            break;
        case -1:
            {
            #if defined(_WIN32)
                const bool isTimeout = WSAGetLastError() == WSAETIMEDOUT;
            #else
                const bool isTimeout = errno == EAGAIN || errno == EWOULDBLOCK;
            #endif
                logger::Error("Recv from [%s:%u] failed [%s].\n", Address, Port, GetNetError());
                if (!isTimeout)
                    return false;
            } break;
        default:
            ptr += recvCnt, left -= recvCnt;
            break;
        }
    }
    return true;
}

void TcpConnection::Close()
{
#if defined(_WIN32)
    closesocket(ClientSocket);
#else
    close(ClientSocket);
#endif
    IsClosed = true;
    //logger::Debug("TCP conenction [%s:%u] closed\n", Address, Port);
}
