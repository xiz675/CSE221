#include "rely.h"

#if defined(_WIN32)
static DWORD WINAPI dummy(LPVOID)
{
    return 0;
}
#else
static void* dummy(void*)
{
    return nullptr;
}
#endif

static void cpu_test4(const uint32_t)
{
    uint64_t time0, time1, timeSum = 0;
#if defined(_WIN32)
    const uint32_t loopCnt = 100;
#else
    const uint32_t loopCnt = 1000;
#endif
    for (uint32_t i = 0; i < loopCnt; ++i)
    {
        FENCE_RDTSC(time0);
    #if defined(_WIN32)
        const auto handle = CreateThread(NULL, 0, dummy, NULL, CREATE_SUSPENDED, NULL);
    #else
        pthread_t pt;
        pthread_create(&pt, nullptr, &dummy, nullptr);
    #endif
        FENCE_RDTSC(time1);
        timeSum += time1 - time0;
    #if defined(_WIN32)
        TerminateThread(handle, 0);
        WaitForSingleObject(handle, INFINITE);
        CloseHandle(handle);
    #else
        pthread_join(pt, nullptr);
    #endif
    }
    PrintResult("create-thread", loopCnt, timeSum);

    timeSum = 0;
#if defined(_WIN32)
    wchar_t name[] = L"cmd.exe";
#endif
    for (uint32_t i = 0; i < loopCnt; ++i)
    {
    #if defined(_WIN32)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));

        FENCE_RDTSC(time0);
        CreateProcess(nullptr, name, nullptr, nullptr, false, CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
    #else
        FENCE_RDTSC(time0);
        const auto pid = fork();
    #endif
        FENCE_RDTSC(time1);
        timeSum += time1 - time0;
    #if defined(_WIN32)
        TerminateProcess(pi.hProcess, 0);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    #else
        if (pid == 0)
            _exit(0);
        int status;
        while (pid != wait(&status))
        { }
    #endif
    }
    PrintResult("create-proc", loopCnt, timeSum);
}

static const uint32_t DummyId = RegistTest("CPU create", '4', true, &cpu_test4, 5);
