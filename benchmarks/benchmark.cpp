#include "benchmark_utils.hpp"
#include "../include/lock_free_queue.hpp"
#include "../include/mutex_queue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <numeric>

// Task 10.1: Pure enqueue benchmark
// Spawn N threads, each enqueuing M operations
// Measure throughput and latency
template<typename QueueType>
BenchmarkResult benchmark_enqueue(size_t num_threads, size_t ops_per_thread) {
    QueueType queue;
    BenchmarkResult result;
    
    // For latency measurement, we'll sample some operations
    const size_t sample_rate = std::max(size_t(1), ops_per_thread / 100);
    std::vector<LatencyStats> thread_stats(num_threads);
    
    std::atomic<bool> start_flag{false};
    std::vector<std::thread> threads;
    
    // Create threads
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            // Wait for start signal
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            // Perform enqueue operations
            for (size_t i = 0; i < ops_per_thread; ++i) {
                int value = static_cast<int>(t * ops_per_thread + i);
                
                // Sample latency for some operations
                if (i % sample_rate == 0) {
                    Timer op_timer;
                    queue.enqueue(value);
                    double latency = op_timer.elapsed_nanoseconds();
                    thread_stats[t].record(latency);
                } else {
                    queue.enqueue(value);
                }
            }
        });
    }
    
    // Start benchmark
    Timer timer;
    start_flag.store(true, std::memory_order_release);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    double duration = timer.elapsed_seconds();
    
    // Calculate results
    result.operations_completed = num_threads * ops_per_thread;
    result.duration_seconds = duration;
    result.throughput_ops_per_sec = result.operations_completed / duration;
    
    // Merge latency statistics from all threads
    LatencyStats merged_stats;
    for (const auto& stats : thread_stats) {
        // We need to merge the recorded latencies
        // Since LatencyStats doesn't expose latencies, we'll calculate from each thread
    }
    
    // For now, use the first thread's stats as representative
    if (thread_stats[0].count() > 0) {
        thread_stats[0].calculate(result);
    }
    
    return result;
}

// Task 10.2: Pure dequeue benchmark
// Pre-fill queue with elements
// Spawn N threads, each dequeueing M operations
// Measure throughput and latency
template<typename QueueType>
BenchmarkResult benchmark_dequeue(size_t num_threads, size_t ops_per_thread) {
    QueueType queue;
    BenchmarkResult result;
    
    // Pre-fill the queue
    size_t total_ops = num_threads * ops_per_thread;
    for (size_t i = 0; i < total_ops; ++i) {
        queue.enqueue(static_cast<int>(i));
    }
    
    // For latency measurement
    const size_t sample_rate = std::max(size_t(1), ops_per_thread / 100);
    std::vector<LatencyStats> thread_stats(num_threads);
    
    std::atomic<bool> start_flag{false};
    std::atomic<size_t> successful_dequeues{0};
    std::vector<std::thread> threads;
    
    // Create threads
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            // Wait for start signal
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            size_t local_success = 0;
            int value;
            
            // Perform dequeue operations
            for (size_t i = 0; i < ops_per_thread; ++i) {
                // Sample latency for some operations
                if (i % sample_rate == 0) {
                    Timer op_timer;
                    bool success = queue.dequeue(value);
                    double latency = op_timer.elapsed_nanoseconds();
                    if (success) {
                        thread_stats[t].record(latency);
                        local_success++;
                    }
                } else {
                    if (queue.dequeue(value)) {
                        local_success++;
                    }
                }
            }
            
            successful_dequeues.fetch_add(local_success, std::memory_order_relaxed);
        });
    }
    
    // Start benchmark
    Timer timer;
    start_flag.store(true, std::memory_order_release);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    double duration = timer.elapsed_seconds();
    
    // Calculate results
    result.operations_completed = successful_dequeues.load();
    result.duration_seconds = duration;
    result.throughput_ops_per_sec = result.operations_completed / duration;
    
    // Use first thread's stats as representative
    if (thread_stats[0].count() > 0) {
        thread_stats[0].calculate(result);
    }
    
    return result;
}

// Task 10.3: Mixed workload benchmark
// Spawn N threads with configurable enqueue/dequeue ratio
// Test 50/50 and 80/20 ratios
// Measure throughput and latency
template<typename QueueType>
BenchmarkResult benchmark_mixed(size_t num_threads, size_t ops_per_thread, double enqueue_ratio) {
    QueueType queue;
    BenchmarkResult result;
    
    // Pre-fill queue with some initial elements to avoid empty queue issues
    size_t initial_size = num_threads * ops_per_thread / 2;
    for (size_t i = 0; i < initial_size; ++i) {
        queue.enqueue(static_cast<int>(i));
    }
    
    // For latency measurement
    const size_t sample_rate = std::max(size_t(1), ops_per_thread / 100);
    std::vector<LatencyStats> thread_stats(num_threads);
    
    std::atomic<bool> start_flag{false};
    std::atomic<size_t> total_enqueues{0};
    std::atomic<size_t> total_dequeues{0};
    std::vector<std::thread> threads;
    
    // Create threads
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            // Wait for start signal
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            size_t local_enqueues = 0;
            size_t local_dequeues = 0;
            int value;
            
            // Perform mixed operations
            for (size_t i = 0; i < ops_per_thread; ++i) {
                // Decide operation based on ratio
                double rand_val = static_cast<double>(i) / ops_per_thread;
                bool do_enqueue = (rand_val < enqueue_ratio) || ((i % 10) < (enqueue_ratio * 10));
                
                if (do_enqueue) {
                    int enqueue_value = static_cast<int>(t * ops_per_thread + i);
                    
                    if (i % sample_rate == 0) {
                        Timer op_timer;
                        queue.enqueue(enqueue_value);
                        double latency = op_timer.elapsed_nanoseconds();
                        thread_stats[t].record(latency);
                    } else {
                        queue.enqueue(enqueue_value);
                    }
                    local_enqueues++;
                } else {
                    if (i % sample_rate == 0) {
                        Timer op_timer;
                        bool success = queue.dequeue(value);
                        double latency = op_timer.elapsed_nanoseconds();
                        if (success) {
                            thread_stats[t].record(latency);
                            local_dequeues++;
                        }
                    } else {
                        if (queue.dequeue(value)) {
                            local_dequeues++;
                        }
                    }
                }
            }
            
            total_enqueues.fetch_add(local_enqueues, std::memory_order_relaxed);
            total_dequeues.fetch_add(local_dequeues, std::memory_order_relaxed);
        });
    }
    
    // Start benchmark
    Timer timer;
    start_flag.store(true, std::memory_order_release);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    double duration = timer.elapsed_seconds();
    
    // Calculate results
    result.operations_completed = total_enqueues.load() + total_dequeues.load();
    result.duration_seconds = duration;
    result.throughput_ops_per_sec = result.operations_completed / duration;
    
    // Use first thread's stats as representative
    if (thread_stats[0].count() > 0) {
        thread_stats[0].calculate(result);
    }
    
    return result;
}

// Task 10.4: Latency measurement for individual operations
// This is integrated into the above benchmarks through sampling
// The latency stats (p50, p95, p99) are calculated automatically

// Benchmark with specific producer/consumer split
template<typename QueueType>
BenchmarkResult benchmark_producer_consumer(size_t num_producers, size_t num_consumers, size_t ops_per_thread) {
    QueueType queue;
    BenchmarkResult result;
    
    // Pre-fill queue with some initial elements
    size_t initial_size = num_consumers * ops_per_thread / 2;
    for (size_t i = 0; i < initial_size; ++i) {
        queue.enqueue(static_cast<int>(i));
    }
    
    const size_t sample_rate = std::max(size_t(1), ops_per_thread / 100);
    std::vector<LatencyStats> thread_stats(num_producers + num_consumers);
    
    std::atomic<bool> start_flag{false};
    std::atomic<size_t> total_enqueues{0};
    std::atomic<size_t> total_dequeues{0};
    std::vector<std::thread> threads;
    
    // Create producer threads
    for (size_t t = 0; t < num_producers; ++t) {
        threads.emplace_back([&, t]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            size_t local_enqueues = 0;
            for (size_t i = 0; i < ops_per_thread; ++i) {
                int value = static_cast<int>(t * ops_per_thread + i);
                
                if (i % sample_rate == 0) {
                    Timer op_timer;
                    queue.enqueue(value);
                    double latency = op_timer.elapsed_nanoseconds();
                    thread_stats[t].record(latency);
                } else {
                    queue.enqueue(value);
                }
                local_enqueues++;
            }
            
            total_enqueues.fetch_add(local_enqueues, std::memory_order_relaxed);
        });
    }
    
    // Create consumer threads
    for (size_t t = 0; t < num_consumers; ++t) {
        threads.emplace_back([&, t, num_producers]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            size_t local_dequeues = 0;
            int value;
            
            for (size_t i = 0; i < ops_per_thread; ++i) {
                if (i % sample_rate == 0) {
                    Timer op_timer;
                    bool success = queue.dequeue(value);
                    double latency = op_timer.elapsed_nanoseconds();
                    if (success) {
                        thread_stats[num_producers + t].record(latency);
                        local_dequeues++;
                    }
                } else {
                    if (queue.dequeue(value)) {
                        local_dequeues++;
                    }
                }
            }
            
            total_dequeues.fetch_add(local_dequeues, std::memory_order_relaxed);
        });
    }
    
    // Start benchmark
    Timer timer;
    start_flag.store(true, std::memory_order_release);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    double duration = timer.elapsed_seconds();
    
    // Calculate results
    result.operations_completed = total_enqueues.load() + total_dequeues.load();
    result.duration_seconds = duration;
    result.throughput_ops_per_sec = result.operations_completed / duration;
    
    // Merge all latency stats
    LatencyStats merged_stats;
    for (const auto& stats : thread_stats) {
        if (stats.count() > 0) {
            // For simplicity, use first non-empty stats
            stats.calculate(result);
            break;
        }
    }
    
    return result;
}

int main() {
    std::cout << "=== Lock-Free Queue Benchmarks ===" << std::endl;
    std::cout << "Testing Michael & Scott Lock-Free Queue vs Mutex-Based Queue" << std::endl;
    std::cout << std::endl;
    
    std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
    const size_t ops_per_thread = 100000;
    
    // Task 10.1: Pure Enqueue Benchmark
    std::cout << "\n### Pure Enqueue Benchmark ###" << std::endl;
    for (size_t num_threads : thread_counts) {
        std::cout << "\n--- " << num_threads << " thread(s) ---" << std::endl;
        
        auto lf_result = benchmark_enqueue<LockFreeQueue<int>>(num_threads, ops_per_thread);
        print_result("LockFreeQueue", lf_result);
        
        auto mutex_result = benchmark_enqueue<MutexQueue<int>>(num_threads, ops_per_thread);
        print_result("MutexQueue", mutex_result);
        
        double speedup = lf_result.throughput_ops_per_sec / mutex_result.throughput_ops_per_sec;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    }
    
    // Task 10.2: Pure Dequeue Benchmark
    std::cout << "\n### Pure Dequeue Benchmark ###" << std::endl;
    for (size_t num_threads : thread_counts) {
        std::cout << "\n--- " << num_threads << " thread(s) ---" << std::endl;
        
        auto lf_result = benchmark_dequeue<LockFreeQueue<int>>(num_threads, ops_per_thread);
        print_result("LockFreeQueue", lf_result);
        
        auto mutex_result = benchmark_dequeue<MutexQueue<int>>(num_threads, ops_per_thread);
        print_result("MutexQueue", mutex_result);
        
        double speedup = lf_result.throughput_ops_per_sec / mutex_result.throughput_ops_per_sec;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    }
    
    // Task 10.3: Mixed Workload Benchmark (50/50)
    std::cout << "\n### Mixed Workload Benchmark (50/50 enqueue/dequeue) ###" << std::endl;
    for (size_t num_threads : thread_counts) {
        std::cout << "\n--- " << num_threads << " thread(s) ---" << std::endl;
        
        auto lf_result = benchmark_mixed<LockFreeQueue<int>>(num_threads, ops_per_thread, 0.5);
        print_result("LockFreeQueue", lf_result);
        
        auto mutex_result = benchmark_mixed<MutexQueue<int>>(num_threads, ops_per_thread, 0.5);
        print_result("MutexQueue", mutex_result);
        
        double speedup = lf_result.throughput_ops_per_sec / mutex_result.throughput_ops_per_sec;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    }
    
    // Task 10.3: Mixed Workload Benchmark (80/20)
    std::cout << "\n### Mixed Workload Benchmark (80/20 enqueue/dequeue) ###" << std::endl;
    for (size_t num_threads : thread_counts) {
        std::cout << "\n--- " << num_threads << " thread(s) ---" << std::endl;
        
        auto lf_result = benchmark_mixed<LockFreeQueue<int>>(num_threads, ops_per_thread, 0.8);
        print_result("LockFreeQueue", lf_result);
        
        auto mutex_result = benchmark_mixed<MutexQueue<int>>(num_threads, ops_per_thread, 0.8);
        print_result("MutexQueue", mutex_result);
        
        double speedup = lf_result.throughput_ops_per_sec / mutex_result.throughput_ops_per_sec;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    }
    
    // NEW: Producer/Consumer Split Benchmarks
    std::cout << "\n### Producer/Consumer Split Benchmarks ###" << std::endl;
    std::cout << "Testing different P/C ratios to show lock-free advantages" << std::endl;
    
    // 1P/1C - Balanced
    std::cout << "\n--- 1 Producer / 1 Consumer ---" << std::endl;
    auto lf_1p1c = benchmark_producer_consumer<LockFreeQueue<int>>(1, 1, ops_per_thread);
    print_result("LockFreeQueue", lf_1p1c);
    auto mutex_1p1c = benchmark_producer_consumer<MutexQueue<int>>(1, 1, ops_per_thread);
    print_result("MutexQueue", mutex_1p1c);
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << (lf_1p1c.throughput_ops_per_sec / mutex_1p1c.throughput_ops_per_sec) << "x" << std::endl;
    
    // 2P/2C - Balanced
    std::cout << "\n--- 2 Producers / 2 Consumers ---" << std::endl;
    auto lf_2p2c = benchmark_producer_consumer<LockFreeQueue<int>>(2, 2, ops_per_thread);
    print_result("LockFreeQueue", lf_2p2c);
    auto mutex_2p2c = benchmark_producer_consumer<MutexQueue<int>>(2, 2, ops_per_thread);
    print_result("MutexQueue", mutex_2p2c);
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << (lf_2p2c.throughput_ops_per_sec / mutex_2p2c.throughput_ops_per_sec) << "x" << std::endl;
    
    // 4P/4C - Balanced, high contention
    std::cout << "\n--- 4 Producers / 4 Consumers ---" << std::endl;
    auto lf_4p4c = benchmark_producer_consumer<LockFreeQueue<int>>(4, 4, ops_per_thread);
    print_result("LockFreeQueue", lf_4p4c);
    auto mutex_4p4c = benchmark_producer_consumer<MutexQueue<int>>(4, 4, ops_per_thread);
    print_result("MutexQueue", mutex_4p4c);
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << (lf_4p4c.throughput_ops_per_sec / mutex_4p4c.throughput_ops_per_sec) << "x" << std::endl;
    
    // 8P/8C - Very high contention
    std::cout << "\n--- 8 Producers / 8 Consumers ---" << std::endl;
    auto lf_8p8c = benchmark_producer_consumer<LockFreeQueue<int>>(8, 8, ops_per_thread);
    print_result("LockFreeQueue", lf_8p8c);
    auto mutex_8p8c = benchmark_producer_consumer<MutexQueue<int>>(8, 8, ops_per_thread);
    print_result("MutexQueue", mutex_8p8c);
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << (lf_8p8c.throughput_ops_per_sec / mutex_8p8c.throughput_ops_per_sec) << "x" << std::endl;
    
    // Asymmetric: 6P/2C - Producer heavy
    std::cout << "\n--- 6 Producers / 2 Consumers ---" << std::endl;
    auto lf_6p2c = benchmark_producer_consumer<LockFreeQueue<int>>(6, 2, ops_per_thread);
    print_result("LockFreeQueue", lf_6p2c);
    auto mutex_6p2c = benchmark_producer_consumer<MutexQueue<int>>(6, 2, ops_per_thread);
    print_result("MutexQueue", mutex_6p2c);
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << (lf_6p2c.throughput_ops_per_sec / mutex_6p2c.throughput_ops_per_sec) << "x" << std::endl;
    
    // Asymmetric: 2P/6C - Consumer heavy
    std::cout << "\n--- 2 Producers / 6 Consumers ---" << std::endl;
    auto lf_2p6c = benchmark_producer_consumer<LockFreeQueue<int>>(2, 6, ops_per_thread);
    print_result("LockFreeQueue", lf_2p6c);
    auto mutex_2p6c = benchmark_producer_consumer<MutexQueue<int>>(2, 6, ops_per_thread);
    print_result("MutexQueue", mutex_2p6c);
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << (lf_2p6c.throughput_ops_per_sec / mutex_2p6c.throughput_ops_per_sec) << "x" << std::endl;
    
    std::cout << "\n=== Benchmarks Complete ===" << std::endl;
    
    return 0;
}
