#include <gtest/gtest.h>
#include "../include/mutex_queue.hpp"
#include <thread>
#include <vector>

// Basic functionality tests for MutexQueue

TEST(MutexQueueTest, ConstructorCreatesEmptyQueue) {
    MutexQueue<int> queue;
    EXPECT_TRUE(queue.empty());
}

TEST(MutexQueueTest, SingleEnqueueDequeue) {
    MutexQueue<int> queue;
    queue.enqueue(42);
    EXPECT_FALSE(queue.empty());
    
    int result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, 42);
    EXPECT_TRUE(queue.empty());
}

TEST(MutexQueueTest, DequeueOnEmptyQueueReturnsFalse) {
    MutexQueue<int> queue;
    int result;
    EXPECT_FALSE(queue.dequeue(result));
}

TEST(MutexQueueTest, MultipleEnqueueDequeue) {
    MutexQueue<int> queue;
    
    // Enqueue multiple values
    for (int i = 0; i < 10; ++i) {
        queue.enqueue(i);
    }
    
    // Dequeue and verify order
    for (int i = 0; i < 10; ++i) {
        int result;
        EXPECT_TRUE(queue.dequeue(result));
        EXPECT_EQ(result, i);
    }
    
    EXPECT_TRUE(queue.empty());
}

TEST(MutexQueueTest, MoveSemantics) {
    MutexQueue<std::string> queue;
    std::string value = "test";
    queue.enqueue(std::move(value));
    
    std::string result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, "test");
}

TEST(MutexQueueTest, ConcurrentEnqueue) {
    MutexQueue<int> queue;
    const int num_threads = 4;
    const int ops_per_thread = 1000;
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&queue, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                queue.enqueue(t * ops_per_thread + i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all elements were enqueued
    int count = 0;
    int result;
    while (queue.dequeue(result)) {
        ++count;
    }
    
    EXPECT_EQ(count, num_threads * ops_per_thread);
}
