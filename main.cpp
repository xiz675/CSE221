#include "rely.h"
#include "OptionParser.hpp"
#include "LoadWorker.hpp"
#include <map>
#include <vector>
#include <array>
#include <chrono>
#include <random>
#include <mutex>
#include <condition_variable>

#if defined(__GNU_LIBRARY__)
#   include <gnu/libc-version.h>
#   pragma message("Compiling with glibc[" STRINGIZE(__GLIBC__) "." STRINGIZE(__GLIBC_MINOR__) "]")
#endif

using std::string;
using std::map;
using std::vector;
using std::array;
using optparse::OptionItem;


void set_affinity(const uint8_t coreId)
{
#if defined(_WIN32)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    const DWORD_PTR mask = UINT64_C(1) << coreId;
    if (SetThreadAffinityMask(GetCurrentThread(), mask) == FALSE)
        EXIT_MSG("fail to set affinity");
#elif defined(__APPLE__)
    thread_affinity_policy_data_t policy = { 1 };
    thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
#else
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(coreId, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1)
        EXIT_MSG("fail to set affinity");
#endif
}

uint64_t get_affinity()
{
#if defined(_WIN32)
    const auto mask = SetThreadAffinityMask(GetCurrentThread(), 0);
    SetThreadAffinityMask(GetCurrentThread(), mask);
    return mask;
#elif defined(__APPLE__)
    int32_t core_count = get_proc_count();
    /*if (sysctlbyname(SYSCTL_CORE_COUNT, &core_count, sizeof(core_count), 0, 0))
        EXIT_MSG("fail to get affinity");*/
    return (UINT64_C(1) << core_count) - 1;
#else
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(mask), &mask) == -1)
        EXIT_MSG("fail to get affinity");
    uint64_t ret = 0;
    for (uint8_t i = 0; i < 64; ++i)
    {
        if (CPU_ISSET(i, &mask))
            ret |= (UINT64_C(1) << i);
    }
    return ret;
#endif
}

void set_priority()
{
#if defined(_WIN32)
    if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
        EXIT_MSG("fail to set priority");
#elif defined(__APPLE__)
#else
    setpriority(PRIO_PROCESS, 0, -20); // with real-time scheduler, this should be ignored, but keep it in case realtime not supported
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(getpid(), SCHED_FIFO, &param);
#endif
}

void ensureFileExists(const std::string& fileName, const size_t size)
{
    FILE * fp = fopen(fileName.c_str(), "rb");
    if (fp == nullptr)
    {
        logger::Warn("need to create empty file of %fGB.\n", size * 1.0 / (1024 * 1024 * 1024));
        fp = fopen(fileName.c_str(), "wb");
        std::vector<uint32_t> dummy(16 * 1048576);// 64MB
        std::random_device seed;
        {
            //truly write to avoid lazy initialization
            for (size_t left = size; left > 0;)
            {
                for (auto& val : dummy)
                    val = seed();
                const auto wsize = std::min(left, dummy.size() * sizeof(uint32_t));
                fwrite(dummy.data(), wsize, 1, fp);
                left -= wsize;
            }
            /*fseek(fp, 2 * 1024 * 1024 * 1024 - 1, SEEK_SET);
            fwrite(fp, 1, 1, fp);*/
        }
        logger::Success("successfully create empty file of %fGB.\n", size * 1.0 / (1024 * 1024 * 1024));
    }
    fclose(fp);
}


namespace
{
struct TestInfo
{
    string Name;
    void(*Func)(const uint32_t);
    uint32_t Warmup;
    bool NeedLoad;
    bool IsCycles;
};
static map<char, TestInfo>& GetTestMap()
{
    static map<char, TestInfo> testMap;
    return testMap;
}
static map<std::pair<const TestInfo*, string>, vector<double>> TEST_RESULT;
static const TestInfo* CUR_TEST = nullptr;
static map<string, string> ExtraArgs;
}

uint32_t RegistTest(const char * name, const char flag, const bool needLoad, void(*func)(const uint32_t), const uint32_t warmup, const bool reportCycle)
{
    GetTestMap()[flag] = TestInfo{ name, func, warmup, needLoad, reportCycle };
    printf("Test [%-22s] registed with flag [%c].\n", name, flag);
    return 0;
}

std::string GetExtraArgument(const std::string& key)
{
    const auto it = ExtraArgs.find(key);
    if (it == ExtraArgs.end())
        return "";
    return it->second;
}


void PrintResult(const std::string& test, const uint32_t loopCount, const uint64_t clocks, const uint64_t, const std::function<void(const double)>& printer)
{
    if (CUR_TEST == nullptr)
        return;
    const double cycles = clocks * 1.0 / loopCount;
    if (printer)
    {
        printer(cycles);
    }
    else
    {
        const char* unit = CUR_TEST->IsCycles ? "cycles" : "ns";
        logger::Info("%-30s [%8d] cost [%10" PRIu64 "] %s, avg [%10.2f] %s each.\n", test, loopCount, clocks, unit, cycles, unit);
    }
    TEST_RESULT[{CUR_TEST, test}].push_back(cycles);
}



int main(int argc, char *argv[])
{
#if defined(_WIN32)
    constexpr auto platform = "Windows";
    {// need to manually enable Win10's support for ASNI color
        const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(handle, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(handle, mode);
    }
#elif defined(__APPLE__)
    constexpr auto platform = "macOS";
#else
    constexpr auto platform = "Linux";
#endif

    array<OptionItem, 9> options
    {
        OptionItem { 't', "test",        "tests that need to be performed",                      optparse::StrArg { "" } },
        OptionItem { 'i', "iteration",   "how many iterations each test need to do",             optparse::UIntArg{ 5 } },
        OptionItem { 'l', "loop",        "how many loops of operation should each iteration do", optparse::UIntArg{ 1000000 } },
        OptionItem { 'w', "worker",      "whether need to launch load worker",                   optparse::BoolArg{ true } },
        OptionItem { 'u', "totime",      "transform final data to time rather than clocks",      optparse::BoolArg{ true } },
        OptionItem { 'o', "output",      "output result as file",                                optparse::BoolArg{ true } },
        OptionItem { 'p', "prefix",      "platform prefix",                                      optparse::StrArg{ platform } },
        OptionItem { 'f', "freq",        "specify real CPU frequency(GHz) for scaling",          optparse::DoubleArg{ -1.0 } },
        OptionItem { 'v', "values",      "extra values for tests",                               optparse::StrMapArg{ {} } },
    };
    if (argc == 1 || !optparse::ParseCommands(options, argc, argv))
    {
        optparse::PrintHelp(options);
        return 0;
    }
    const string tests = options[0].StrValue;
    const uint32_t iterations = (uint32_t)options[1].UIntValue;
    const uint32_t loopCount = (uint32_t)options[2].UIntValue;
    bool globalNeedLoad = options[3].BoolValue;
    const bool transTime = options[4].BoolValue;
    const bool outfile = options[5].BoolValue;
    string prefix = options[6].StrValue;
    const double freq = options[7].DoubleValue;
    ExtraArgs = std::move(options[8].StrMapValue);
    
    if (globalNeedLoad)
    /*#if defined(__APPLE__)
        globalNeedLoad = false;
    #else*/
        globalNeedLoad = get_proc_count() > 3;
    //#endif
    else
        logger::Warn("%s\n", "manually turned off load worker!");
    if (tests.empty())
        return 0;

    {//prepare thread-aff binding
    #if defined(__GNU_LIBRARY__)
        logger::Debug("glibc version[%s]\n", gnu_get_libc_version());
    #endif
        set_priority();
        const auto oldMask = get_affinity();
        logger::Debug("original aff:%" PRIX64 "\n", oldMask);
        const auto procCnt = get_proc_count();
        set_affinity(procCnt - 1);
        const auto newMask = get_affinity();
        logger::Debug("new aff:%" PRIX64 "\n", newMask);
    }

    LoadWorker loadWorker(globalNeedLoad);

    double freqNs = 0;
    double freqScale = 1;
    {
        loadWorker.Run();
        logger::Warn("%s\n", "calculate frequency...");
        const auto from = std::chrono::high_resolution_clock::now();
        uint64_t clock0, clock1;
        FENCE_RDTSC(clock0);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        FENCE_RDTSC(clock1);
        const auto to = std::chrono::high_resolution_clock::now();
        const auto elapseNs = std::chrono::duration_cast<std::chrono::nanoseconds>(to - from).count();
        const auto tscfreq = (clock1 - clock0) * 1.0 / elapseNs;
        freqNs = elapseNs * 1.0 / (clock1 - clock0);
        logger::Success("freqency of FENCE_RDTSC is %.3f GHz, %.3fns/cycle \n", tscfreq, freqNs);
        if (freq > 0)
            freqScale = freq / tscfreq;
        loadWorker.Stop();
    }

    const auto& testMap = GetTestMap();
    try
    {
        for (const auto testFlag : tests)
        {
            const auto it = testMap.find(testFlag);
            if (it == testMap.end())
            {
                logger::Error("flag [%c] has no corresponding test!\n", testFlag);
                continue;
            }
            const auto& testInfo = it->second;
            logger::Info("======Test [%s]======\n", testInfo.Name);
            if (testInfo.NeedLoad)
                loadWorker.Run();

            logger::Warn("Warm-up stage with [%u] iterations\n", testInfo.Warmup);
            CUR_TEST = nullptr;
            for (uint32_t i = 0; i < testInfo.Warmup; ++i)
                testInfo.Func(loopCount);
            CUR_TEST = &testInfo;
            for (uint32_t i = 0; i < iterations; ++i)
            {
                logger::Success("===iteration %d\n", i);
                testInfo.Func(loopCount);
            }

            if (testInfo.NeedLoad)
                loadWorker.Stop();
        }
        logger::Success("%s\n", "All test done.");
    }
    catch (const std::runtime_error& e)
    {
        logger::Error("%s\n", e.what());
    }

    if (outfile)
    {
        logger::Warn("%s\n", "generate result...");
        const auto fname = prefix + std::to_string(__rdtsc()) + ".test.csv";
        const auto fp = fopen(fname.c_str(), "wt");
        fprintf(fp, "unit:,%s\n", transTime ? "ns" : "cycles");
        fprintf(fp, "Test, SubTest,");
        for (uint32_t i = 0; i < iterations; ++i)
            fprintf(fp, "Round%d,", i);
        fprintf(fp, "Average\n");
        for (const auto& p : TEST_RESULT)
        {
            const auto& test = *p.first.first;
            fprintf(fp, "%s,%s,", test.Name.c_str(), p.first.second.c_str());
            double sum = 0;
            for (const auto t : p.second)
            {
                const double val = test.IsCycles ? (transTime ? t * freqNs : t * freqScale) : t;
                fprintf(fp, "%f,", val);
                sum += val;
            }
            fprintf(fp, "%f\n", sum / p.second.size());
        }
        fclose(fp);
        logger::Success("write to [%s] !\n", fname);
    }

#if defined(_WIN32)
    getchar();
#endif
    return 0;
}