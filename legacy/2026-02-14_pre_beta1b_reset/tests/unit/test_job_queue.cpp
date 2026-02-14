/**
 * @file test_job_queue.cpp
 * @brief Unit tests for the job queue
 */

#include <gtest/gtest.h>
#include "core/job_queue.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

using namespace scratchrobin;

class JobQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<JobQueue>();
    }
    
    void TearDown() override {
        if (queue_) {
            queue_->Stop();
        }
    }
    
    std::unique_ptr<JobQueue> queue_;
};

TEST_F(JobQueueTest, SubmitAndExecuteJob) {
    std::atomic<bool> executed{false};
    
    JobHandle handle = queue_->Submit([&executed](JobHandle&) {
        executed = true;
    });
    
    // Wait for job to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(executed);
}

TEST_F(JobQueueTest, MultipleJobs) {
    std::atomic<int> counter{0};
    
    std::vector<JobHandle> handles;
    for (int i = 0; i < 10; ++i) {
        handles.push_back(queue_->Submit([&counter](JobHandle&) {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }));
    }
    
    // Wait for all jobs
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(counter, 10);
}

TEST_F(JobQueueTest, CancelJob) {
    std::atomic<bool> started{false};
    std::atomic<bool> completed{false};
    
    JobHandle handle = queue_->Submit([&](JobHandle& h) {
        started = true;
        // Simulate long-running work with cancellation check
        for (int i = 0; i < 100; ++i) {
            if (h.IsCanceled()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        completed = true;
    });
    
    // Wait a bit for job to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Cancel the job
    handle.Cancel();
    
    EXPECT_TRUE(handle.IsCanceled());
    
    // Wait and verify job didn't complete normally
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_TRUE(started);
    EXPECT_FALSE(completed);
}

TEST_F(JobQueueTest, ConcurrentJobSubmission) {
    std::atomic<int> completed{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([this, &completed]() {
            for (int i = 0; i < 20; ++i) {
                auto handle = queue_->Submit([&completed](JobHandle&) {
                    completed++;
                });
                (void)handle; // Suppress unused warning
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Wait for all jobs to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(completed, 100);
}

TEST_F(JobQueueTest, StopWaitsForJobs) {
    std::atomic<bool> job_completed{false};
    std::atomic<int> job_count{0};
    
    // Submit several jobs
    for (int i = 0; i < 5; ++i) {
        queue_->Submit([&](JobHandle&) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            job_count++;
            if (job_count == 5) {
                job_completed = true;
            }
        });
    }
    
    // Give jobs time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Stop should wait for queued jobs to complete
    queue_->Stop();
    
    EXPECT_TRUE(job_completed);
    EXPECT_EQ(job_count, 5);
}

TEST_F(JobQueueTest, JobHandleDefaultConstructible) {
    // Default constructed handle should be valid but empty
    JobHandle handle;
    
    // Should be able to call methods without crash
    EXPECT_FALSE(handle.IsCanceled());
    
    // Cancel on empty handle should not crash
    handle.Cancel();
}

TEST_F(JobQueueTest, JobReceivesHandleReference) {
    std::atomic<bool> received_valid_handle{false};
    
    JobHandle submitted = queue_->Submit([&received_valid_handle](JobHandle& h) {
        // Job receives a reference to its handle
        // We can check if it's valid by using it
        received_valid_handle = true;
        h.IsCanceled(); // Should not crash
    });
    
    (void)submitted;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(received_valid_handle);
}

TEST_F(JobQueueTest, SequentialJobExecution) {
    std::vector<int> execution_order;
    std::mutex mutex;
    
    // Submit jobs sequentially
    for (int i = 0; i < 5; ++i) {
        queue_->Submit([&execution_order, &mutex, i](JobHandle&) {
            std::lock_guard<std::mutex> lock(mutex);
            execution_order.push_back(i);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Wait for all
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Verify all jobs executed
    EXPECT_EQ(execution_order.size(), 5);
}

TEST_F(JobQueueTest, SetCancelCallback) {
    std::atomic<bool> callback_called{false};
    
    JobHandle handle = queue_->Submit([&](JobHandle& h) {
        // Set up cancel callback
        h.SetCancelCallback([&callback_called]() {
            callback_called = true;
        });
        
        // Wait until cancelled or timeout
        for (int i = 0; i < 50; ++i) {
            if (h.IsCanceled()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Wait for job to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Cancel should trigger callback
    handle.Cancel();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(callback_called);
}

TEST_F(JobQueueTest, QueueDestructor) {
    std::atomic<int> completed{0};
    
    {
        JobQueue local_queue;
        
        for (int i = 0; i < 5; ++i) {
            local_queue.Submit([&completed](JobHandle&) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                completed++;
            });
        }
        
        // Queue destructor should wait for jobs
    }
    
    EXPECT_EQ(completed, 5);
}
