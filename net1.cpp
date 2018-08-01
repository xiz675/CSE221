#include "rely.h"
#include "TcpUtil.h"


static void networkRTT(const uint32_t loopCount)
{
    constexpr uint16_t port = 8998;
    constexpr uint32_t authKey = 0xabadcafe;
    constexpr size_t size = 64;
    const auto remoteAddr = GetExtraArgument("RemoteAddr");
    const std::string localAddr = "127.0.0.1";

    for (const auto& addr : { localAddr, remoteAddr })
    {
        TcpConnection conn(addr, port);
        conn.SendData(&authKey, sizeof(authKey));
        conn.SendData(&size, sizeof(size));
        conn.SendData(&size, sizeof(size));
        conn.SendData(&loopCount, sizeof(loopCount));
        conn.SendData(&authKey, sizeof(authKey));

        uint8_t clientBuffer[size];
        uint8_t ackBuffer[size];
        uint64_t rtt_time = 0;
        uint32_t count = 0;

        for (; count < loopCount && rtt_time < 5000000000; count++) // less than 5s
        {
            const auto time1 = std::chrono::high_resolution_clock::now();
            conn.SendData(clientBuffer, size);
            conn.ReceiveData(ackBuffer, size);
            const auto time2 = std::chrono::high_resolution_clock::now();
            rtt_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        }

        const auto testname = "RTT-" + addr;
        PrintResult(testname, count, rtt_time);
    }
}

static const uint32_t DummyId = RegistTest("network RTT", '9', true, &networkRTT, 0, false);
