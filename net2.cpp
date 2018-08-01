#include "rely.h"
#include "TcpUtil.h"
#include <vector>

static void peakBandwidth(const uint32_t loopCount)
{
    constexpr uint16_t port = 8998;
    constexpr uint32_t authKey = 0xabadcafe;
    constexpr size_t largeSize = 4 * 1048576, smallSize = 64;
    const auto remoteAddr = GetExtraArgument("RemoteAddr");
    const std::string localAddr = "127.0.0.1";

    std::vector<uint8_t> largeBuffer(largeSize);
    uint8_t smallBuffer[smallSize];

    for (const auto& addr : { localAddr, remoteAddr })
    {
        {
            uint64_t time = 0;
            uint32_t count = 0;
            TcpConnection conn(addr, port);
            conn.SendData(&authKey, sizeof(authKey));
            conn.SendData(&largeSize, sizeof(largeSize));
            conn.SendData(&smallSize, sizeof(smallSize));
            conn.SendData(&loopCount, sizeof(loopCount));
            conn.SendData(&authKey, sizeof(authKey));

            for (; count < loopCount && time < 5000000000; count++) // less than 5s
            {
                const auto time1 = std::chrono::high_resolution_clock::now();
                conn.SendData(largeBuffer.data(), largeSize);
                conn.ReceiveData(smallBuffer, smallSize);
                const auto time2 = std::chrono::high_resolution_clock::now();
                time += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
            }

            const auto testname = "netBW-down-" + addr;
            PrintResult(testname, count, time, 0, [&](const double perSize)
            {
                logger::Debug("download speed: assume %f MB/s\n", largeSize / perSize * 1024);
            });
        }
        {
            uint64_t time = 0;
            uint32_t count = 0;
            TcpConnection conn(addr, port);
            conn.SendData(&authKey, sizeof(authKey));
            conn.SendData(&smallSize, sizeof(smallSize));
            conn.SendData(&largeSize, sizeof(largeSize));
            conn.SendData(&loopCount, sizeof(loopCount));
            conn.SendData(&authKey, sizeof(authKey));

            for (; count < loopCount && time < 5000000000; count++) // less than 5s
            {
                const auto time1 = std::chrono::high_resolution_clock::now();
                conn.SendData(smallBuffer, smallSize);
                conn.ReceiveData(largeBuffer.data(), largeSize);
                const auto time2 = std::chrono::high_resolution_clock::now();
                time += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
            }

            const auto testname = "netBW-up-" + addr;
            PrintResult(testname, count, time, 0, [&](const double perSize)
            {
                logger::Debug("upload speed: assume %f MB/s\n", largeSize / perSize * 1024);
            });
        }
    }
}

static const uint32_t DummyId = RegistTest("Peak Bandwidth", 'a', true, &peakBandwidth, 0, false);
