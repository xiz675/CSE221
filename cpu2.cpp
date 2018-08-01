#include "rely.h"

#if defined(_WIN32)
# pragma optimize("tsg", off)
# pragma warning(disable: 4100)
# define NOOPT 
#elif defined(__clang__)
# if defined(__APPLE__)
#   define NOOPT __attribute__ ((optnone))
# else
#   pragma clang optimize off
#   define NOOPT 
# endif
#else
# pragma GCC push_options
# pragma GCC optimize ("O0")
# define NOOPT 
#endif

static forcenoinline void func0() NOOPT { ;}

static forcenoinline void func1(int a) NOOPT { ;}

static forcenoinline void func2(int a, int b) NOOPT { ;}

static forcenoinline void func3(int a, int b, int c) NOOPT { ;}

static forcenoinline void func4(int a, int b, int c, int d) NOOPT { ;}

static forcenoinline void func5(int a, int b, int c, int d, int e) NOOPT { ;}

static forcenoinline void func6(int a, int b, int c, int d, int e, int f) NOOPT { ;}

static forcenoinline void func7(int a, int b, int c, int d, int e, int f, int g) NOOPT { ;}

#if defined(_WIN32)
# pragma warning(default: 4100)
# pragma optimize("", on)
# undef NOOPT 
#elif defined(__clang__)
# if defined(__APPLE__)
#   undef NOOPT
# else
#   pragma clang optimize on
#   undef NOOPT 
# endif
#else
# pragma GCC pop_options
# undef NOOPT 
#endif

static void cpu_test2(const uint32_t loopCount)
{
    uint64_t time0, time1;
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func0();
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-0", loopCount, time1 - time0);
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func1(0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-1", loopCount, time1 - time0);
    
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func2(0, 0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-2", loopCount, time1 - time0);
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func3(0, 0, 0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-3", loopCount, time1 - time0);
   
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func4(0, 0, 0, 0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-4", loopCount, time1 - time0);
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func5(0, 0, 0, 0, 0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-5", loopCount, time1 - time0);
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func6(0, 0, 0, 0, 0, 0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-6", loopCount, time1 - time0);
    
    FENCE_RDTSC(time0);
    for (uint32_t i = 0; i < loopCount; ++i)
    {
        func7(0, 0, 0, 0, 0, 0, 0);
    }
    FENCE_RDTSC(time1);
    PrintResult("procedure-call-7", loopCount, time1 - time0);
    
    
}

static const uint32_t DummyId = RegistTest("CPU func-call", '2', true, &cpu_test2, 15);
