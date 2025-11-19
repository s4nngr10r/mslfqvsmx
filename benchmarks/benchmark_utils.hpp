#ifndef BENCHMARK_UTILS_HPP
#define BENCHMARK_UTILS_HPP

#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

// Structure to store benchmark results
struct BenchmarkResult {
    size_t operations_completed;
    double duration_seconds;
    double throughput_ops_per_sec;
    double min_latency_ns;
    double max_latency_ns;
    double avg_latency_ns;
    double p50_latency_ns;
    double p95_latency_ns;
    double p99_latency_ns;
    
    BenchmarkResult() 
        : operations_completed(0)
        , duration_seconds(0.0)
        , throughput_ops_per_sec(0.0)
        , min_latency_ns(0.0)
        , max_latency_ns(0.0)
        , avg_latency_ns(0.0)
        , p50_latency_ns(0.0)
        , p95_latency_ns(0.0)
        , p99_latency_ns(0.0)
    {}
};

// High-resolution timer class
class Timer {
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    
    TimePoint start_time;
    
public:
    Timer() : start_time(Clock::now()) {}
    
    void reset() {
        start_time = Clock::now();
    }
    
    double elapsed_seconds() const {
        auto end_time = Clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        return duration.count() / 1e9;
    }
    
    double elapsed_nanoseconds() const {
        auto end_time = Clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        return static_cast<double>(duration.count());
    }
};

// Latency statistics calculator
class LatencyStats {
private:
    std::vector<double> latencies_ns;
    
public:
    void record(double latency_ns) {
        latencies_ns.push_back(latency_ns);
    }
    
    void calculate(BenchmarkResult& result) const {
        if (latencies_ns.empty()) {
            return;
        }
        
        // Need to make a copy for sorting since this is const
        std::vector<double> sorted_latencies = latencies_ns;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        // Min and max
        result.min_latency_ns = sorted_latencies.front();
        result.max_latency_ns = sorted_latencies.back();
        
        // Average
        double sum = 0.0;
        for (double lat : sorted_latencies) {
            sum += lat;
        }
        result.avg_latency_ns = sum / sorted_latencies.size();
        
        // Percentiles (using sorted copy)
        result.p50_latency_ns = percentile_from_sorted(sorted_latencies, 50.0);
        result.p95_latency_ns = percentile_from_sorted(sorted_latencies, 95.0);
        result.p99_latency_ns = percentile_from_sorted(sorted_latencies, 99.0);
    }
    
    void clear() {
        latencies_ns.clear();
    }
    
    size_t count() const {
        return latencies_ns.size();
    }
    
private:
    static double percentile_from_sorted(const std::vector<double>& sorted_data, double p) {
        if (sorted_data.empty()) {
            return 0.0;
        }
        
        double index = (p / 100.0) * (sorted_data.size() - 1);
        size_t lower = static_cast<size_t>(std::floor(index));
        size_t upper = static_cast<size_t>(std::ceil(index));
        
        if (lower == upper) {
            return sorted_data[lower];
        }
        
        double weight = index - lower;
        return sorted_data[lower] * (1.0 - weight) + sorted_data[upper] * weight;
    }
};

// Print benchmark results in formatted output
inline void print_result(const std::string& benchmark_name, const BenchmarkResult& result) {
    std::cout << "\n=== " << benchmark_name << " ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Operations completed: " << result.operations_completed << std::endl;
    std::cout << "Duration: " << result.duration_seconds << " seconds" << std::endl;
    std::cout << "Throughput: " << result.throughput_ops_per_sec << " ops/sec" << std::endl;
    
    if (result.min_latency_ns > 0.0) {
        std::cout << "\nLatency Statistics (nanoseconds):" << std::endl;
        std::cout << "  Min:     " << std::setw(12) << result.min_latency_ns << " ns" << std::endl;
        std::cout << "  Average: " << std::setw(12) << result.avg_latency_ns << " ns" << std::endl;
        std::cout << "  P50:     " << std::setw(12) << result.p50_latency_ns << " ns" << std::endl;
        std::cout << "  P95:     " << std::setw(12) << result.p95_latency_ns << " ns" << std::endl;
        std::cout << "  P99:     " << std::setw(12) << result.p99_latency_ns << " ns" << std::endl;
        std::cout << "  Max:     " << std::setw(12) << result.max_latency_ns << " ns" << std::endl;
    }
    
    std::cout << std::endl;
}

#endif // BENCHMARK_UTILS_HPP
