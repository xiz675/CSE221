#include "rely.h"

namespace
{
struct TestData
{
    std::atomic<uint64_t> dummy;
    uint8_t padding0[56];
};
}

static void cpu_test1(const uint32_t loopCount)
{
    uint64_t time0, time1;

    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
    #if defined(_WIN32)
        __nop();
    #else
        asm("");
    #endif
    }
    FENCE_RDTSC(time1);
    PrintResult("plain-loop", loopCount, time1 - time0);

    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        __rdtsc();
    }
    FENCE_RDTSC(time1);
    PrintResult("rdtsc-loop", loopCount, time1 - time0);

    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        FENCE_RDTSC(time1);
    }
    FENCE_RDTSC(time1);
    PrintResult("fence-rdtsc", loopCount, time1 - time0);

    TestData data;
    data.dummy = 0;
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        FENCE_RDTSC(data.dummy);
    }
    FENCE_RDTSC(time1);
    PrintResult("rdtsc-atomic", loopCount, time1 - time0);
}

static const uint32_t DummyId = RegistTest("CPU measure", '1', true, &cpu_test1, 15);
