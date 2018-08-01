#include "rely.h"
#include "RawFile.hpp"
#include <exception>
#include <array>
#include <random>


static void pageFaultTest(const uint32_t)
{
    const std::string fileName = "largefile.dat";
    ensureFileExists(fileName, 2 * 1024 * 1048576u); // 2GB
    std::random_device seed;
    std::mt19937 rander(seed());
    constexpr uint32_t basicSize = 1048576 * 16;
    std::uniform_int_distribution<uint32_t> dist(0, basicSize);
    FileMapping mapping(fileName);
    
    std::uniform_int_distribution<uint32_t> tmpdist(basicSize, basicSize);
    const uint32_t step = tmpdist(rander);
    uint64_t totalTime = 0;
    uint32_t dummy = 0, totalCount = 0;
    const uint32_t idxLimit = (uint32_t)(mapping.FileSize / sizeof(uint32_t));
    const uint32_t* ptr = mapping.Data;
    for (uint32_t i = 0; i < 500; ++i)
    {
        const auto time1 = std::chrono::high_resolution_clock::now();
        for (uint32_t idx = dist(rander); idx < idxLimit; idx += step)
        {
            dummy += ptr[idx];
            totalCount++;
        }
        const auto time2 = std::chrono::high_resolution_clock::now();
        const auto elapseNs = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        totalTime += elapseNs;
        if (totalTime > 5000000000) //larger than 5s
        {
            logger::Warn("%s\n", "detect HDD slow read, skip to accelerate.");
            break;
        }
    }
    PrintResult("Page fault", totalCount, totalTime, dummy);
}

static const uint32_t DummyId = RegistTest("Page Fault", '8', true, &pageFaultTest, 1, false);
