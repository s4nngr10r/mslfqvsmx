#ifndef MUTEX_QUEUE_HPP
#define MUTEX_QUEUE_HPP

#include <deque>
#include <mutex>

template<typename T>
class MutexQueue {
private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
    
public:
    MutexQueue() = default;
    ~MutexQueue() = default;
    
    void enqueue(const T& value);
    void enqueue(T&& value);
    bool dequeue(T& result);
    bool empty() const;
    
    // Prevent copying
    MutexQueue(const MutexQueue&) = delete;
    MutexQueue& operator=(const MutexQueue&) = delete;
};

// Template implementation

template<typename T>
void MutexQueue<T>::enqueue(const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(value);
}

template<typename T>
void MutexQueue<T>::enqueue(T&& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(value));
}

template<typename T>
bool MutexQueue<T>::dequeue(T& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    result = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

template<typename T>
bool MutexQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

#endif // MUTEX_QUEUE_HPP
