/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_JOB_QUEUE_H
#define SCRATCHROBIN_JOB_QUEUE_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace scratchrobin {

class JobHandle {
public:
    JobHandle() = default;

    void Cancel();
    bool IsCanceled() const;
    void SetCancelCallback(std::function<void()> callback);

private:
    struct State {
        std::atomic<bool> canceled{false};
        std::mutex mutex;
        std::function<void()> cancel_callback;
    };

    explicit JobHandle(std::shared_ptr<State> state) : state_(std::move(state)) {}

    std::shared_ptr<State> state_;

    friend class JobQueue;
};

class JobQueue {
public:
    using Job = std::function<void(JobHandle&)>;

    JobQueue();
    ~JobQueue();

    JobHandle Submit(Job job);
    void Stop();

private:
    struct QueuedJob {
        Job job;
        std::shared_ptr<JobHandle::State> state;
    };

    void WorkerLoop();

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<QueuedJob> queue_;
    std::thread worker_;
    bool stopping_ = false;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_JOB_QUEUE_H
