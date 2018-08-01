#include "rely.h"
#include "TcpUtil.h"


static void tcpOverhead(const uint32_t)
{   
    constexpr uint16_t port = 8998;
    const auto remoteAddr = GetExtraArgument("RemoteAddr");
    const std::string localAddr = "127.0.0.1";
    
    for (const auto& addr : { localAddr, remoteAddr })
    {
        uint64_t setup_time = 0;
        uint64_t teardown_time = 0;
        uint32_t count = 0;
        for (; count < 1000; count++)
        {
            const auto time1 = std::chrono::high_resolution_clock::now();
            TcpConnection conn(addr, port);
            const auto time2 = std::chrono::high_resolution_clock::now();
            setup_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
            const auto time3 = std::chrono::high_resolution_clock::now();
            conn.Close();
            const auto time4 = std::chrono::high_resolution_clock::now();
            teardown_time += std::chrono::duration_cast<std::chrono::nanoseconds>(time4 - time3).count();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        const auto testname = "netSetUp-" + addr;
        PrintResult(testname, count, setup_time);

        const auto testname2 = "netTearDowm" + addr;
        PrintResult(testname2, count, teardown_time);
    }
}

static const uint32_t DummyId = RegistTest("TCP overhead", 'b', true, &tcpOverhead, 0, false);
