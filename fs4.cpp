#include "rely.h"
#include "RawFile.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>
#include <atomic>
#include <array>
#include <numeric>

static void fsContention(const uint32_t)
{
    const std::string fileName = "largefile.dat";
    constexpr size_t totalSize = 2 * 1024 * 1048576u;
    ensureFileExists(fileName, totalSize); // 2GB
    constexpr uint32_t regions[] = { 2,4,8,16,32 };
    constexpr auto largest = GetLastElement(regions) * 4096u;
    constexpr uint32_t thrCount[] = { 2,4,16,64 };

    constexpr auto maxThrCount = GetLastElement(thrCount);
    uint64_t times[maxThrCount] = { 0 };
    uint32_t counts[maxThrCount] = { 0 };
    std::thread threads[maxThrCount] = {};
    std::unique_ptr<RawFile> files[maxThrCount] = {};

    std::atomic<uint64_t> readCount;

    for (uint32_t i = 0; i < maxThrCount; ++i)
    {
        files[i].reset(new RawFile(fileName, true, true));
    }

    for (const auto tCount : thrCount)
    {
        const auto readyState = UINT64_MAX >> (64 - tCount);
        const uint64_t partSize = (totalSize / tCount) & 0xfffffffffffff000;
        for (const auto region : regions)
        {
            const uint64_t maxSize = region * 1048576u;
            const uint32_t loopCount = (uint32_t)(maxSize / 4096);
            readCount = 0;
            std::mutex mtx;
            std::condition_variable cv;
            for (uint32_t i = 0; i < tCount; ++i)
            {
                threads[i] = std::thread([=, &files, &readCount, &times, &counts, &mtx, &cv]()
                {
                    const RawFile& file = *files[i];
                    file.Seek(partSize * i);

                    std::array<uint8_t, 4096> dummy{ 0 };
                    uint64_t totalTime = 0;
                    uint32_t count = 0;
                    {
                        readCount |= (UINT64_C(1) << i);
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [&] { return readCount == 0; });
                    }
                    for (; count < loopCount && totalTime < 1000000000; count++) // no longer than 1s
                    {
                        const auto time1 = std::chrono::high_resolution_clock::now();
                        file.Read(dummy.data(), 4096);
                        const auto time2 = std::chrono::high_resolution_clock::now();
                        totalTime += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
                    }
                    times[i] = totalTime, counts[i] = count;
                });
            }
            {
                while (readCount != readyState)
                    YIELD_THREAD();
                mtx.lock();
                readCount = 0;
                mtx.unlock();
                cv.notify_all();
            }
            for (uint32_t i = 0; i < tCount; ++i)
                threads[i].join();

            const uint64_t totalTime = std::accumulate(times, times + tCount, (uint64_t)0);
            const uint32_t totalCount = std::accumulate(counts, counts + tCount, (uint32_t)0);
            const auto testname = "fscon-seq-" + std::to_string(tCount) + "-" + std::to_string(region);
            PrintResult(testname, totalCount, totalTime, 0, [&](const double perBlock)
            {
                logger::Debug("seq %2u thread read %4u MB, assume speed : %f MB/s\n", tCount, region, 4096 * 1024 / perBlock);
            });

        }
    }

}

static const uint32_t DummyId = RegistTest("Filesystem Contention", 'f', false, &fsContention, 0, false);
