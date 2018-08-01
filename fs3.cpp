#include "rely.h"
#include "RawFile.hpp"
#include <vector>
#include <array>
#include <random>
#include <memory>


static void fsRemoteRead(const uint32_t loopCount)
{
    const std::string fileName = GetExtraArgument("RemoteFileName");
    ensureFileExists(fileName, 2 * 1024 * 1048576u); // 2GB
    RawFile file(fileName, true);
    constexpr uint32_t regions[] = { 2,8,32,128,512 };
    std::array<uint8_t, 4096> dummy{ 0 };

    for (const auto region : regions)
    {
        uint64_t totalTime = 0;
        uint32_t count = 0;
        const size_t maxSize = region * 1048576u;
        file.Seek(0);
        for (uint64_t pos = 0; count < loopCount && totalTime < 1000000000; count++) // no longer than 1s
        {
            if (pos >= maxSize)
                file.Seek(pos = 0);
            const auto time1 = std::chrono::high_resolution_clock::now();
            file.Read(dummy.data(), 4096); pos += 4096;
            const auto time2 = std::chrono::high_resolution_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        }
        const auto testname = "rfsRead-seq-" + std::to_string(region);
        PrintResult(testname, count, totalTime, 0, [&](const double perBlock)
        {
            logger::Debug("seq read %4uMB file, assume read speed : %f MB/s\n", region, 4096 * 1024 / perBlock);
        });
    }

    std::random_device seed;
    std::mt19937 rander(seed());
    for (const auto region : regions)
    {
        std::uniform_int_distribution<uint64_t> dist(0, region * 1048576u / 4096);
        uint64_t totalTime = 0;
        uint32_t count = 0;
        for (; count < loopCount && totalTime < 1000000000; count++) // no longer than 1s
        {
            file.Seek(dist(rander) * 4096);
            const auto time1 = std::chrono::high_resolution_clock::now();
            file.Read(dummy.data(), 4096);
            const auto time2 = std::chrono::high_resolution_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        }
        const auto testname = "rfsRead-ran-" + std::to_string(region);
        PrintResult(testname, count, totalTime, 0, [&](const double perBlock)
        {
            logger::Debug("random read %4uMB file, assume read speed : %f MB/s\n", region, 4096 * 1024 / perBlock);
        });
    }
}

static const uint32_t DummyId = RegistTest("Remote FS Read", 'e', false, &fsRemoteRead, 1, false);
