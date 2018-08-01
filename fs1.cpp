#include "rely.h"
#include <vector>
#include <algorithm>

static void fsCacheSize(const uint32_t)
{
    const std::string fileName = GetExtraArgument("LargeFileCache");
    ensureFileExists(fileName, 10 * 1024 * UINT64_C(1048576)); // 10GB
    constexpr uint32_t regions[] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192 };
    FILE * fp = fopen(fileName.c_str(), "rb");
    std::vector<uint8_t> dummy(1024 * 1048576u);
    for (const auto region : regions)
    {
        uint64_t totalTime = 0;
        FSeek64(fp, 0, SEEK_SET);
        const auto time1 = std::chrono::high_resolution_clock::now();
        for (uint32_t i = 0; i < region; i += 1024)
            fread(dummy.data(), 1048576, std::min(1024u, region - i), fp);
        const auto time2 = std::chrono::high_resolution_clock::now();
        totalTime += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        const auto testname = "fscache-" + std::to_string(region);
        PrintResult(testname, region, totalTime, 0, [&](const double perMB)
        {
            logger::Debug("read %4u MB, assume read speed : %f GB/s\n", region, 1048576 / perMB);
        });
    }
    fclose(fp);
}

static const uint32_t DummyId = RegistTest("Filesystem Cache", 'c', false, &fsCacheSize, 0, false);
