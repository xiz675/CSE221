#pragma once

#if defined(_WIN32)
#   define FSeek64(fp, offset, whence) _fseeki64(fp, offset, whence)
#   define FTell64(fp) _ftelli64(fp)
#else
#   define _FILE_OFFSET_VITS 64
#   include <unistd.h>
#   include <cerrno>
#   define FSeek64(fp, offset, whence) fseeko(fp, offset, whence)
#   define FTell64(fp) ftello(fp)
#endif

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <cinttypes>
#include <thread>
#include <functional>
#include <atomic>
#if defined(_WIN32)
# define NOMINMAX 1
# define WIN32_LEAN_AND_MEAN 1
# include <Windows.h>
#else
#   include <limits.h> // for glibc detection
#   include <pthread.h>
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/resource.h>
#   include <sys/wait.h>
# if defined(__APPLE__)
#   include <sys/sysctl.h>
#   include <mach/mach_types.h>
#   include <mach/thread_act.h>
# else
#   include <sched.h>
# endif
#endif
#include "Logger.hpp"

#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#if defined(_MSC_VER)
#   define forceinline __forceinline
#   define forcenoinline __declspec(noinline)
#elif defined(__GNUC__)
#   define forceinline __inline__ __attribute__((always_inline))
#   define forcenoinline __attribute__((noinline))
#else
#   define forceinline inline
#   define forcenoinline 
#endif

#if defined(__GNUC__)
# if __GNUC__ <= 4 && __GNUC_MINOR__ <= 5 && !defined(__APPLE__)
#   error GCC Version too low to use this header, at least gcc 4.5.0
# endif
# if defined(__INTEL_COMPILER)
#   if __INTEL_COMPILER < 1500
#     error ICC Version too low to use this header, at least icc 15.0
#   endif
# endif
# include <x86intrin.h>
#elif defined(_MSC_VER)
# if _MSC_VER < 1200
#   error MSVC Version too low to use this header, at least msvc 12.0
# endif
# include <intrin.h>
# if (_M_IX86_FP == 2) && !defined(__SSE2__)
#   define __SSE2__ 1
# endif
#else
#error Unknown compiler, not supported by this header
#endif

#if defined(__APPLE__)
#   include <malloc/malloc.h>
forceinline void* apple_malloc_align(const size_t size, const size_t align)
{
    void* ptr = nullptr;
    if (posix_memalign(&ptr, align, size))
        return nullptr;
    return ptr;
}
#   define malloc_align(size, align) apple_malloc_align((size), (align))
#   define free_align(ptr) free(ptr)
#elif defined(__GNUC__)
#   include <malloc.h>
#   define malloc_align(size, align) memalign((align), (size))
#   define free_align(ptr) free(ptr)
#elif defined(_MSC_VER)
#   define malloc_align(size, align) _aligned_malloc((size), (align))
#   define free_align(ptr) _aligned_free(ptr)
#endif

#define RDTSC(value) { value = __rdtsc(); }

forceinline uint64_t get_rdtscp()
{
    uint32_t dummy__;
    return __rdtscp(&dummy__);
}

#if defined(PURETSC)
#   define FENCE_RDTSC(value) \
    { \
        _mm_lfence(); \
        value = __rdtsc(); \
    }
#else
#   define FENCE_RDTSC(value) \
    { \
        uint32_t dummy__; \
        value = __rdtscp(&dummy__); \
    }
#endif

#if defined(_WIN32)
#   define YIELD_THREAD() SwitchToThread()
#else
#   define YIELD_THREAD() sched_yield()
#endif

template<typename T, typename... Args>
forceinline void EXIT_MSG(const T& msg, const Args&... args)
{
    logger::Error(msg, args...);
    throw std::runtime_error("vital error, exit...");
}
template<typename T>
forceinline void EXIT_MSG(const T& msg)
{
    EXIT_MSG("%s", msg);
}


forceinline uint8_t get_proc_count()
{
    return (uint8_t)std::thread::hardware_concurrency();
}
void set_affinity(const uint8_t coreId);
uint64_t get_affinity();
void set_priority();

template<typename T, size_t N>
constexpr inline T& GetLastElement(T(&arr)[N])
{
    return arr[N - 1];
}

void ensureFileExists(const std::string& fileName, const size_t size);

//macro for intellisense
#if defined(_WIN32)
///<summary>Regist a test</summary>  
///<param name="name">test name</param>
///<param name="flag">single char to identify the test</param>
///<param name="needLoad">if need additional load thread</param>
///<param name="func">pointer to the test function with type [void(const uint32_t iteration)]</param>
///<param name="warmup">warmup iterations</param>
///<param name="reportCycle">if the reported time is cpu cycles or other unconvertable measurement (like real time in ns)</param>
///<returns>dummy integer</returns>
uint32_t RegistTest(const char *name, const char flag, const bool needLoad, void(*func)(const uint32_t), const uint32_t warmup, const bool reportCycle = true);
#else
/**
 * @brief Regist a test
 * @param name      test name
 * @param flag      single char to identify the test
 * @param needLoad  if need additional load thread
 * @param func      pointer to the test function with type [void(const uint32_t iteration)]
 * @param warmup    warmup iterations
 * @param reportCycle  if the reported time is cpu cycles or other unconvertable measurement (like real time in ns)
 * @return dummy integer
 */
uint32_t RegistTest(const char *name, const char flag, const bool needLoad, void(*func)(const uint32_t), const uint32_t warmup, const bool reportCycle = true);
#endif

std::string GetExtraArgument(const std::string& key);

void PrintResult(const std::string& test, const uint32_t loopCount, const uint64_t cycles, const uint64_t dummy = 0, const std::function<void(const double)>& printer = {});


