#include "rely.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>

using std::vector;
using std::wstring;


namespace
{
struct TimePair
{
    uint64_t time;
    uint8_t id;
    bool operator<(const TimePair& rhs)const noexcept { return time < rhs.time; }
};
}

static void buildResult(const char* test, const vector<uint64_t>& time0, const vector<uint64_t>& time1)
{
    vector<TimePair> times;
    for (auto cur = ++time0.cbegin(); cur != time0.cend(); ++cur)
        times.push_back({ *cur, 0 });
    for (auto cur = ++time1.cbegin(); cur != time1.cend(); ++cur)
        times.push_back({ *cur, 1 });

    uint32_t validCnt = 0;
    uint64_t totalTime = 0;
    std::sort(times.begin(), times.end());
    for (size_t i = 1; i < times.size(); i++)
    {
        if (times[i].id != times[i - 1].id)
        {
            totalTime += times[i].time - times[i - 1].time;
            validCnt++;
        }
    }

    PrintResult(test, validCnt, totalTime);
}

static void testThread(const uint32_t loopCount)
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    std::condition_variable cv;
    std::vector<uint64_t> time0(loopCount, 0);
    std::vector<uint64_t> time1(loopCount, 0);

    auto thr = std::thread([&, loopCount]
    {
        set_affinity(get_proc_count() - 1);
        {
            std::unique_lock<std::mutex> lock0(mtx);
            cv.notify_all();
        }
        for (uint32_t i = 0; i < loopCount; i++)
        {
            FENCE_RDTSC(time1[i]);
            YIELD_THREAD();
        }
    });

    cv.wait(lock);
    for (uint32_t i = 0; i < loopCount; i++)
    {
        FENCE_RDTSC(time0[i]);
        YIELD_THREAD();
    }

    thr.join();

    buildResult("thread-switch", time0, time1);
}

#if defined(_WIN32)
static void cpu_test5_child(const uint32_t loopCount)
{
    const auto handle = GetStdHandle(STD_ERROR_HANDLE);
    std::vector<uint64_t> time(loopCount, 0);
    WriteFile(handle, &loopCount, 1, nullptr, nullptr); // use as signal

    for (uint32_t i = 0; i < loopCount; i++)
    {
        FENCE_RDTSC(time[i]);
        YIELD_THREAD();
    }
    
    WriteFile(handle, time.data(), (DWORD)(time.size() * sizeof(uint64_t)), nullptr, nullptr);
}
static void testProc(const uint32_t loopCount)
{
    std::vector<uint64_t> time0(loopCount, 0);
    std::vector<uint64_t> time1(loopCount, 0);

    HANDLE hRead;
    PROCESS_INFORMATION pi;
    {
        HANDLE hWrite;
        SECURITY_ATTRIBUTES sa;
        ZeroMemory(&sa, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        CreatePipe(&hRead, &hWrite, &sa, sizeof(uint64_t) * loopCount * 2);

        wchar_t name[1024] = { 0 };
        GetModuleFileName(NULL, name, 1024);
        wstring cmd(name);
        cmd += L" -tX -i1 -ooff -woff -l" + std::to_wstring(loopCount);
       
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));
        si.hStdError = hWrite;
        si.dwFlags = STARTF_USESTDHANDLES;
        CreateProcess(nullptr, cmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
        CloseHandle(hWrite);
    }

    uint8_t dummy;
    ReadFile(hRead, &dummy, 1, nullptr, nullptr);

    for (uint32_t i = 0; i < loopCount; i++)
    {
        FENCE_RDTSC(time0[i]);
        YIELD_THREAD();
    }

    ReadFile(hRead, time1.data(), (DWORD)(time1.size() * sizeof(uint64_t)), nullptr, nullptr);

    {
        TerminateProcess(pi.hProcess, 0);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hRead);
    }
    buildResult("proc-switch", time0, time1);
}
static const uint32_t DummyId2 = RegistTest("CPU ctx-switch-child", 'X', true, &cpu_test5_child, 5);
#else
static void testProc(const uint32_t loopCount)
{
    std::vector<uint64_t> time0(loopCount, 0);
    std::vector<uint64_t> time1(loopCount, 0);
    int pipeid[2];
    pipe(pipeid); // default blocking
    const auto pid = fork();
    if (pid == 0)
    {
        set_priority();
        set_affinity(get_proc_count() - 1);
        std::vector<uint64_t> time(loopCount, 0);
        close(pipeid[0]); // close reader
        write(pipeid[1], &pid, 1); // use as signal

        for (uint32_t i = 0; i < loopCount; i++)
        {
            FENCE_RDTSC(time[i]);
            YIELD_THREAD();
        }

        {
            size_t off = 0, left = time.size() * sizeof(uint64_t);
            const uint8_t* ptr = (const uint8_t*)time.data();
            ssize_t ret;
            do
            {
                ret = write(pipeid[1], ptr + off, left);
                off += ret, left -= ret;
            } while (left > 0 && ret > 0);
        }
        _exit(0);
    }
    else
    {
        close(pipeid[1]);
        uint8_t dummy;
        read(pipeid[0], &dummy, 1);

        for (uint32_t i = 0; i < loopCount; i++)
        {
            FENCE_RDTSC(time0[i]);
            YIELD_THREAD();
        }

        {
            size_t off = 0, left = time1.size() * sizeof(uint64_t);
            uint8_t* ptr = (uint8_t*)time1.data();
            ssize_t ret;
            do
            {
                ret = read(pipeid[0], ptr + off, left);
                off += ret, left -= ret;
            } while (left > 0 && ret > 0);
        }

        int status;
        while (pid != wait(&status))
        { }

        buildResult("proc-switch", time0, time1);
    }
}
#endif

static void cpu_test5(const uint32_t loopCount)
{
    testThread(loopCount / 10);

    testProc(loopCount / 10);
}

static const uint32_t DummyId = RegistTest("CPU ctx-switch", '5', true, &cpu_test5, 5);
