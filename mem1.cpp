#include "rely.h"
#include <chrono>



static void cacheLatencyTest(const uint32_t loopCount)
{
    constexpr uint32_t array_size[] = { 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2 * 1024, 4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024 };
    constexpr uint32_t step_size[] = { 32, 128, 1024, 8 * 1024 };
    uint32_t* mem = (uint32_t*)malloc_align(GetLastElement(array_size) * 1024, 32);
    
    for (const auto stepSize : step_size)
    {
        const uint32_t step = stepSize / sizeof(uint32_t);
        for (const auto arrSize : array_size)
        {
            const uint32_t interger_num = arrSize * 1024 / sizeof(uint32_t);
            for (uint32_t i = 0; i < interger_num; i++)
            {
                const auto desire = i + step;
                mem[i] = desire >= interger_num ? (desire + 1) % interger_num : desire;
            }
            uint32_t dummySum = 0;
            const auto time1 = std::chrono::high_resolution_clock::now();
            for (uint32_t i = 0, idx = 0; i < loopCount; i++)
            {
                idx = mem[idx];
                dummySum += idx;
            }
            const auto time2 = std::chrono::high_resolution_clock::now();
            const uint64_t latency_time = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();

            const auto testname = "memLatency-" + std::to_string(arrSize * 1024) + "-" + std::to_string(stepSize);
            PrintResult(testname, loopCount, latency_time, dummySum);
        }
    }
    free_align(mem);
}

static const uint32_t DummyId = RegistTest("Cache Latency", '6', true, &cacheLatencyTest, 2, false);
