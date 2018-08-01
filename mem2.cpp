#include "rely.h"
#include <chrono>


static void ramBandwidthTest(const uint32_t)
{
    constexpr size_t count = 32 * 1024 * 1024;
    constexpr uint32_t loops = 100;
    auto* mem = (uint32_t*)malloc_align(count * sizeof(uint32_t), 32);
    const auto dat = _mm256_set1_epi32(10);
    uint64_t timeStore = 0, timeLoad = 0;
    for (uint32_t i = 0; i < loops; ++i)
    {
        auto* ptr = (__m256i*)mem;
        const auto time1 = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < count; j += 64, ptr += 8)
        {
            _mm256_stream_si256(ptr + 0, dat);
            _mm256_stream_si256(ptr + 1, dat);
            _mm256_stream_si256(ptr + 2, dat);
            _mm256_stream_si256(ptr + 3, dat);
            _mm256_stream_si256(ptr + 4, dat);
            _mm256_stream_si256(ptr + 5, dat);
            _mm256_stream_si256(ptr + 6, dat);
            _mm256_stream_si256(ptr + 7, dat);
        }
        _mm_mfence();
        const auto time2 = std::chrono::high_resolution_clock::now();
        const auto elapseNs = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        timeStore += elapseNs;
    }
    PrintResult("memory store(AVX)", loops, timeStore, 0, [=](const double perLoop)
    {
        logger::Debug("assume store speed : %f GB/s\n", count * sizeof(float) / perLoop);
    });
    __m256i d1 = dat, d2 = dat, d3 = dat, d4 = dat, d5 = dat, d6 = dat, d7 = dat, d8 = dat;
    for (uint32_t i = 0; i < loops; ++i)
    {
        auto* ptr = (const __m256i*)mem;
        const auto time1 = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < count; j += 64, ptr += 8)
        {
            d1 = _mm256_add_epi32(d1, _mm256_stream_load_si256(ptr + 0));
            d2 = _mm256_add_epi32(d2, _mm256_stream_load_si256(ptr + 1));
            d3 = _mm256_add_epi32(d3, _mm256_stream_load_si256(ptr + 2));
            d4 = _mm256_add_epi32(d4, _mm256_stream_load_si256(ptr + 3));
            d5 = _mm256_add_epi32(d5, _mm256_stream_load_si256(ptr + 4));
            d6 = _mm256_add_epi32(d6, _mm256_stream_load_si256(ptr + 5));
            d7 = _mm256_add_epi32(d7, _mm256_stream_load_si256(ptr + 6));
            d8 = _mm256_add_epi32(d8, _mm256_stream_load_si256(ptr + 7));
        }
        _mm_mfence();
        const auto time2 = std::chrono::high_resolution_clock::now();
        const auto elapseNs = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count();
        timeLoad += elapseNs;
    }
    const __m256i d0 = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(d1, d2), _mm256_add_epi32(d1, d2)), _mm256_add_epi32(_mm256_add_epi32(d1, d2), _mm256_add_epi32(d1, d2)));
    PrintResult("memory load(AVX)", loops, timeLoad, *(const uint32_t*)&d0, [=](const double perLoop) 
    {
        logger::Debug("assume load speed : %f GB/s\n", count * sizeof(float) / perLoop);
    });
    free_align(mem);

}

static const uint32_t DummyId = RegistTest("RAM Bandwidth", '7', true, &ramBandwidthTest, 5, false);
