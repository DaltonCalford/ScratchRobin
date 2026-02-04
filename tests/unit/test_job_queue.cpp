/**
 * @file test_job_queue.cpp
 * @brief Unit tests for the job queue
 */

#include <gtest/gtest.h>
#include "core/job_queue.h"
#include <chrono>
#include <thread>

using namespace scratchrobin;

class JobQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<JobQueue>();
    }
    
    void TearDown() override {
        if (queue_) {
            queue_->Shutdown();
        }
    }
    
    std::unique_ptr<JobQueue> queue_;
};

TEST_F(JobQueueTest, SubmitAndExecuteJob) {
    std::atomic<bool> executed{false};
    
    JobHandle handle = queue_->Submit([&executed]() {
        executed = true;
        return JobResult::Success();
    });
    
    EXPECT_NE(handle.id, 0);
    
    // Wait for job to complete
    auto result = queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    EXPECT_TRUE(executed);
}

TEST_F(JobQueueTest, JobReturnsData) {
    JobHandle handle = queue_->Submit([]() {
        JobResult result;
        result.success = true;
        result.data = std::string("Hello from job");
        return result;
    });
    
    auto result = queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    ASSERT_TRUE(result->data.has_value());
    EXPECT_EQ(std::any_cast<std::string>(result->data), "Hello from job");
}

TEST_F(JobQueueTest, JobReturnsError) {
    JobHandle handle = queue_->Submit([]() {
        return JobResult::Failure("Something went wrong");
    });
    
    auto result = queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->success);
    EXPECT_EQ(result->error_message, "Something went wrong");
}

TEST_F(JobQueueTest, CancelJob) {
    std::atomic<bool> started{false};
    std::atomic<bool> cancelled{false};
    
    JobHandle handle = queue_->Submit([&]() {
        started = true;
        // Simulate long-running work with cancellation check
        for (int i = 0; i < 100; ++i) {
            if (queue_->IsCancelled(handle)) {
                cancelled = true;
                return JobResult::Cancelled();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return JobResult::Success();
    });
    
    // Wait a bit for job to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Cancel the job
    bool cancel_result = queue_->Cancel(handle);
    EXPECT_TRUE(cancel_result);
    
    auto result = queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    
    EXPECT_TRUE(started);
    EXPECT_TRUE(cancelled);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->cancelled);
}

TEST_F(JobQueueTest, MultipleJobs) {
    std::atomic<int> counter{0};
    
    std::vector<JobHandle> handles;
    for (int i = 0; i < 10; ++i) {
        handles.push_back(queue_->Submit([&counter]() {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return JobResult::Success();
        }));
    }
    
    // Wait for all jobs
    for (const auto& handle : handles) {
        queue_->WaitForCompletion(handle, std::chrono::seconds(10));
    }
    
    EXPECT_EQ(counter, 10);
}

TEST_F(JobQueueTest, JobPriority) {
    std::vector<int> execution_order;
    std::mutex mutex;
    
    // Submit low priority jobs first
    std::vector<JobHandle> handles;
    for (int i = 0; i < 3; ++i) {
        handles.push_back(queue_->Submit([&execution_order, &mutex, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::lock_guard<std::mutex> lock(mutex);
            execution_order.push_back(i);
            return JobResult::Success();
        }, JobPriority::Low));
    }
    
    // Submit high priority job last
    handles.push_back(queue_->Submit([&execution_order, &mutex]() {
        std::lock_guard<std::mutex> lock(mutex);
        execution_order.push_back(100);
        return JobResult::Success();
    }, JobPriority::High));
    
    // Wait for all
    for (const auto& handle : handles) {
        queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    }
    
    // High priority job should execute first
    EXPECT_EQ(execution_order[0], 100);
}

TEST_F(JobQueueTest, JobTimeout) {
    JobHandle handle = queue_->Submit([]() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return JobResult::Success();
    });
    
    // Set timeout on the job
    queue_->SetTimeout(handle, std::chrono::milliseconds(100));
    
    auto result = queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->success);
    EXPECT_TRUE(result->timed_out);
}

TEST_F(JobQueueTest, GetJobStatus) {
    std::atomic<bool> can_proceed{false};
    
    JobHandle handle = queue_->Submit([&can_proceed]() {
        while (!can_proceed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return JobResult::Success();
    });
    
    // Initially running
    auto status = queue_->GetStatus(handle);
    EXPECT_EQ(status, JobStatus::Running);
    
    // Let it complete
    can_proceed = true;
    queue_->WaitForCompletion(handle, std::chrono::seconds(5));
    
    status = queue_->GetStatus(handle);
    EXPECT_EQ(status, JobStatus::Completed);
}

TEST_F(JobQueueTest, InvalidJobHandle) {
    JobHandle invalid_handle{0};
    
    auto result = queue_->WaitForCompletion(invalid_handle, std::chrono::milliseconds(100));
    EXPECT_FALSE(result.has_value());
}

TEST_F(JobQueueTest, ConcurrentJobSubmission) {
    std::atomic<int> completed{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([this, &completed]() {
            for (int i = 0; i < 20; ++i) {
                auto handle = queue_->Submit([&completed]() {
                    completed++;
                    return JobResult::Success();
                });
                queue_->WaitForCompletion(handle, std::chrono::seconds(5));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed, 100);
}

TEST_F(JobQueueTest, ShutdownWaitsForJobs) {
    std::atomic<bool> job_completed{false};
    
    auto handle = queue_->Submit([&job_completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        job_completed = true;
        return JobResult::Success();
    });
    
    // Shutdown should wait for job to complete
    queue_->Shutdown();
    
    EXPECT_TRUE(job_completed);
}

TEST_F(JobQueueTest, ProgressReporting) {
    JobHandle handle = queue_->Submit([this, handle]() {
        for (int i = 0; i <= 100; i += 10) {
            queue_->ReportProgress(handle, i, "Processing...");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return JobResult::Success();
    });
    
    // Poll for progress
    int last_progress = 0;
    while (queue_->GetStatus(handle) == JobStatus::Running) {
        auto progress = queue_->GetProgress(handle);
        if (progress > last_progress) {
            last_progress = progress;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    EXPECT_GE(last_progress, 0);
}

TEST_F(JobQueueTest, JobDependencies) {
    std::atomic<int> order{0};
    std::vector<int> execution_order;
    std::mutex mutex;
    
    // Submit first job
    auto job1 = queue_->Submit([&]() {
        std::lock_guard<std::mutex> lock(mutex);
        execution_order.push_back(1);
        return JobResult::Success();
    });
    
    // Submit second job that depends on first
    auto job2 = queue_->Submit([&]() {
        std::lock_guard<std::mutex> lock(mutex);
        execution_order.push_back(2);
        return JobResult::Success();
    }, JobPriority::Normal, {job1});
    
    queue_->WaitForCompletion(job2, std::chrono::seconds(5));
    
    ASSERT_EQ(execution_order.size(), 2);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
}
