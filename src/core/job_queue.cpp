/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "job_queue.h"

namespace scratchrobin {

void JobHandle::Cancel() {
    if (!state_) {
        return;
    }
    state_->canceled.store(true);

    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        callback = state_->cancel_callback;
    }
    if (callback) {
        callback();
    }
}

bool JobHandle::IsCanceled() const {
    if (!state_) {
        return false;
    }
    return state_->canceled.load();
}

void JobHandle::SetCancelCallback(std::function<void()> callback) {
    if (!state_) {
        return;
    }
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->cancel_callback = std::move(callback);
}

JobQueue::JobQueue() {
    worker_ = std::thread(&JobQueue::WorkerLoop, this);
}

JobQueue::~JobQueue() {
    Stop();
}

JobHandle JobQueue::Submit(Job job) {
    auto state = std::make_shared<JobHandle::State>();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(QueuedJob{std::move(job), state});
    }
    cv_.notify_one();
    return JobHandle(state);
}

void JobQueue::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void JobQueue::WorkerLoop() {
    while (true) {
        QueuedJob job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&]() { return stopping_ || !queue_.empty(); });
            if (stopping_ && queue_.empty()) {
                break;
            }
            job = std::move(queue_.front());
            queue_.pop();
        }

        JobHandle handle(job.state);
        if (handle.IsCanceled()) {
            continue;
        }
        if (job.job) {
            job.job(handle);
        }
    }
}

} // namespace scratchrobin
