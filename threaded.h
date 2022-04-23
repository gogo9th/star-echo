#pragma once

#include <thread>
#include <filesystem>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "log.hpp"


template<typename Workitem, typename Worker>
class ThreadedWorker
{
    ThreadedWorker(const ThreadedWorker &) = delete;
    ThreadedWorker operator=(const ThreadedWorker &) = delete;

public:
    ThreadedWorker(const std::vector<Workitem> & items, std::unique_ptr<Worker> & worker, int maxThreads = 0)
        : items_(items)
        , worker_(std::move(worker))
    {
        maxThreads_ = maxThreads > 0 ? maxThreads : std::thread::hardware_concurrency();

        msg() << "Using " << maxThreads_ << " CPU threads...";
        for (int i = 0; i < maxThreads_; i++)
        {
            threads_.push_back(std::thread(&ThreadedWorker::threadRun, this, i));
        }
    }

    ~ThreadedWorker()
    {
        {
            std::unique_lock lock(sleeping_mtx_);
            terminate_ = true;
        }
        sleeping_.notify_all();
        for (auto & thread : threads_)
        {
            //if (thread.joinable())
                thread.join();
        }
    }

    void waitForDone()
    {
        if (items_.empty())
        {
            return;
        }

        do
        {
            {
                std::lock_guard lock(mtx_);
                if (inputIdx_ >= items_.size())
                {
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } while (true);
    }


private:
    bool next(Workitem & item)
    {
        std::lock_guard lock(mtx_);

        if (inputIdx_ >= items_.size())
        {
            return false;
        }

        item = items_[inputIdx_++];
        return true;
    }

    void threadRun(int threadIndex)
    {
        while (!terminate_)
        {
            Workitem item;
            if (next(item))
            {
                std::string r = (*worker_)(item);
                {
                    std::lock_guard lock(runMtx_);
                    msg() << "[CPU " << threadIndex + 1 << "] " << ++runIdx_ << '/' << items_.size() << "  " << r;
                }

                // more work to do
                continue;
            }

            // no work, sleep
            {
                std::unique_lock lock(sleeping_mtx_);
                if (terminate_)
                {
                    break;
                }
                sleeping_.wait(lock);
            }
        }
    }


    int maxThreads_;
    std::vector<std::thread>    threads_;
    std::condition_variable     sleeping_;
    std::mutex                  sleeping_mtx_;
    std::atomic_bool            terminate_ = false;

    std::mutex                  mtx_;
    std::vector<Workitem>       items_;
    int                         inputIdx_ = 0;

    std::mutex                  runMtx_;
    int                         runIdx_ = 0;

    std::unique_ptr<Worker>     worker_;
};
