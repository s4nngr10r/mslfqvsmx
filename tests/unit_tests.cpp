#include <gtest/gtest.h>
#include <string>
#include "lock_free_queue.hpp"

// Placeholder test to verify build system works
TEST(LockFreeQueueTest, BuildSystemWorks) {
    EXPECT_TRUE(true);
}

// Test that the queue can be constructed and destroyed
TEST(LockFreeQueueTest, ConstructorDestructor) {
    LockFreeQueue<int>* queue = new LockFreeQueue<int>();
    delete queue;
    EXPECT_TRUE(true); // If we get here, constructor and destructor worked
}

// Test dequeue on empty queue
TEST(LockFreeQueueTest, DequeueEmptyQueue) {
    LockFreeQueue<int> queue;
    int result;
    EXPECT_FALSE(queue.dequeue(result));
}

// Test single enqueue and dequeue
TEST(LockFreeQueueTest, SingleEnqueueDequeue) {
    LockFreeQueue<int> queue;
    queue.enqueue(42);
    int result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, 42);
}

// Test multiple enqueue and dequeue
TEST(LockFreeQueueTest, MultipleEnqueueDequeue) {
    LockFreeQueue<int> queue;
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    
    int result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, 1);
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, 2);
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, 3);
    EXPECT_FALSE(queue.dequeue(result)); // Queue should be empty now
}

// Test empty() method on empty queue
TEST(LockFreeQueueTest, EmptyOnEmptyQueue) {
    LockFreeQueue<int> queue;
    EXPECT_TRUE(queue.empty());
}

// Test empty() method on non-empty queue
TEST(LockFreeQueueTest, EmptyOnNonEmptyQueue) {
    LockFreeQueue<int> queue;
    queue.enqueue(42);
    EXPECT_FALSE(queue.empty());
}

// Test empty() method after enqueue and dequeue
TEST(LockFreeQueueTest, EmptyAfterEnqueueDequeue) {
    LockFreeQueue<int> queue;
    queue.enqueue(1);
    queue.enqueue(2);
    EXPECT_FALSE(queue.empty());
    
    int result;
    queue.dequeue(result);
    EXPECT_FALSE(queue.empty()); // Still one element
    
    queue.dequeue(result);
    EXPECT_TRUE(queue.empty()); // Now empty
}

// Test with std::string type
TEST(LockFreeQueueTest, StringType) {
    LockFreeQueue<std::string> queue;
    EXPECT_TRUE(queue.empty());
    
    queue.enqueue("hello");
    queue.enqueue("world");
    EXPECT_FALSE(queue.empty());
    
    std::string result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, "hello");
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, "world");
    EXPECT_FALSE(queue.dequeue(result));
    EXPECT_TRUE(queue.empty());
}

// Custom struct for testing
struct CustomStruct {
    int id;
    double value;
    
    CustomStruct() : id(0), value(0.0) {}
    CustomStruct(int i, double v) : id(i), value(v) {}
    
    bool operator==(const CustomStruct& other) const {
        return id == other.id && value == other.value;
    }
};

// Test with custom struct type
TEST(LockFreeQueueTest, CustomStructType) {
    LockFreeQueue<CustomStruct> queue;
    EXPECT_TRUE(queue.empty());
    
    CustomStruct s1(1, 1.5);
    CustomStruct s2(2, 2.5);
    CustomStruct s3(3, 3.5);
    
    queue.enqueue(s1);
    queue.enqueue(s2);
    queue.enqueue(s3);
    EXPECT_FALSE(queue.empty());
    
    CustomStruct result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, s1);
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, s2);
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, s3);
    EXPECT_FALSE(queue.dequeue(result));
    EXPECT_TRUE(queue.empty());
}

// Test move semantics with std::string
TEST(LockFreeQueueTest, MoveSemantics) {
    LockFreeQueue<std::string> queue;
    
    std::string s1 = "movable string";
    queue.enqueue(std::move(s1));
    
    std::string result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, "movable string");
}

// Additional tests will be added in subsequent tasks
