#pragma once
#include "rely.h"
#include "Logger.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>


class LoadWorker
{
    std::mutex MtxWorker;
    std::condition_variable CvWorker;
    std::atomic<bool> ShouldRun{ false };
    const bool IsNeeded;
public:
    LoadWorker(const bool isNeeded) : IsNeeded(isNeeded)
    {
        if (IsNeeded)
        {
            std::unique_lock<std::mutex> lock(MtxWorker);
            std::thread(Worker, std::ref(MtxWorker), std::ref(CvWorker), std::ref(ShouldRun))
                .detach();
            CvWorker.wait(lock);
        }
    }

    void Run()
    {
        if (IsNeeded)
        {
            std::unique_lock<std::mutex> lock(MtxWorker);
            ShouldRun = true;
            CvWorker.notify_all();
        }
        else
            logger::Warn("%s\n", "Environment not compatible to run a dummy load worker.");
    }

    void Stop()
    {
        if (IsNeeded)
        {
            ShouldRun = false;
            std::unique_lock<std::mutex> lock(MtxWorker); // wait until worker resumed
        }
    }
private:
#if defined(_WIN32)
#   pragma optimize("tsg", off)
#else
#   pragma GCC push_options
#   pragma GCC optimize ("O1")
#endif
    static void EmptyLoop()
    {
        for (uint32_t i = UINT32_MAX; i--;) // about 1 second
        {
        }
    }
#if defined(_WIN32)
#   pragma optimize("", on)
#else
#   pragma GCC pop_options
#endif
    static void Worker(std::mutex& mtxRun, std::condition_variable& cvMain, const std::atomic<bool>& flagRun)
    {
        std::unique_lock<std::mutex> lock(mtxRun);
        set_priority();
        //logger::Debug("original aff:%" PRIX64 "\n", get_affinity());
        const auto procCnt = get_proc_count();
        set_affinity(procCnt - 3);
        //logger::Debug("new aff:%" PRIX64 "\n", get_affinity());
        logger::Debug("dummy load worker initialized at core %d\n", procCnt - 3);
        cvMain.notify_one();
        while (true)
        {
            if (!flagRun)
            {
                //logger::Debug("%s\n", "load worker sleep");
                cvMain.wait(lock, [&] { return flagRun.load(); });
                //logger::Debug("%s\n", "load worker wakeup");
                EmptyLoop();
            }
        }
    }
};