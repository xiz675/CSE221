#include "rely.h"

#if defined(_WIN32)
namespace
{
static auto GetFunc()
{
    return (long(NTAPI*)(const void*))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtDisplayString");
}
}
#endif

static void cpu_test3(const uint32_t loopCount)
{
    uint64_t total_time = 0;
#if defined(_WIN32)
    const auto func = GetFunc();
    const uint32_t loopCnt = loopCount / 10;
    for (uint32_t i = 0; i < loopCount; i++)
    {
        uint64_t time0, time1;
        FENCE_RDTSC(time0);
        func(nullptr);
        //CloseHandle(nullptr);
        FENCE_RDTSC(time1);
        total_time += time1 - time0;
    }
#else
    const uint32_t loopCnt = 1000;
    for (uint32_t i = 0; i < loopCnt; i++)
    {
        pid_t pid = fork();
        if (pid == -1) 
            EXIT_MSG("Create process failed\n");
        
        if (pid == 0) 
        {
            uint64_t time0, time1;
            FENCE_RDTSC(time0);
            getpid();
            FENCE_RDTSC(time1);
            exit((int) (time1 - time0));
        }
        else
        {
            int32_t status;
            wait(&status);
            total_time += (*(const uint32_t*)&status >> 8);
        }
    }
#endif
    PrintResult("system-call", loopCnt, total_time);
}

static const uint32_t DummyId = RegistTest("CPU system-call", '3', true, &cpu_test3, 5);
