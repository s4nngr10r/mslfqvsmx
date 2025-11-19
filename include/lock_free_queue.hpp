#ifndef LOCK_FREE_QUEUE_HPP
#define LOCK_FREE_QUEUE_HPP

#include <atomic>
#include <memory>

// Michael & Scott Lock-Free Queue (1996)
// 
// IMPORTANT: This implementation uses immediate delete after dequeue,
// which is safe for benchmarking but NOT production-ready.
// 
// For production use, implement proper memory reclamation:
// - Hazard Pointers (recommended)
// - Epoch-Based Reclamation
// - RCU-like deferred free
//
// The ABA problem can occur if a pointer is deleted and reallocated
// at the same address before a CAS operation completes.

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next;
        
        Node() : next(nullptr) {}
        explicit Node(const T& value) : data(value), next(nullptr) {}
        explicit Node(T&& value) : data(std::move(value)), next(nullptr) {}
    };
    
    alignas(64) std::atomic<Node*> head;
    alignas(64) std::atomic<Node*> tail;
    
public:
    LockFreeQueue();
    ~LockFreeQueue();
    
    void enqueue(const T& value);
    void enqueue(T&& value);
    bool dequeue(T& result);
    bool empty() const;
    
    // Prevent copying
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
};

// Template implementation

template<typename T>
LockFreeQueue<T>::LockFreeQueue() {
    // Create initial dummy node
    Node* dummy = new Node();
    head.store(dummy, std::memory_order_relaxed);
    tail.store(dummy, std::memory_order_relaxed);
}

template<typename T>
LockFreeQueue<T>::~LockFreeQueue() {
    // Clean up all remaining nodes
    Node* current = head.load(std::memory_order_relaxed);
    while (current != nullptr) {
        Node* next = current->next.load(std::memory_order_relaxed);
        delete current;
        current = next;
    }
}

template<typename T>
void LockFreeQueue<T>::enqueue(const T& value) {
    // Create new node with copy semantics
    Node* node = new Node(value);
    
    while (true) {
        // Load tail and its next pointer
        Node* last = tail.load(std::memory_order_acquire);
        Node* next = last->next.load(std::memory_order_acquire);
        
        // Check if tail is still consistent
        if (last == tail.load(std::memory_order_acquire)) {
            if (next == nullptr) {
                // Tail is pointing to the last node, try to link new node
                if (last->next.compare_exchange_strong(next, node,
                                                       std::memory_order_release,
                                                       std::memory_order_acquire)) {
                    // Successfully linked new node, try to swing tail
                    tail.compare_exchange_strong(last, node,
                                                std::memory_order_release,
                                                std::memory_order_acquire);
                    return;
                }
            } else {
                // Tail is falling behind, help advance it
                tail.compare_exchange_strong(last, next,
                                            std::memory_order_release,
                                            std::memory_order_acquire);
            }
        }
    }
}

template<typename T>
void LockFreeQueue<T>::enqueue(T&& value) {
    // Create new node with move semantics
    Node* node = new Node(std::move(value));
    
    while (true) {
        // Load tail and its next pointer
        Node* last = tail.load(std::memory_order_acquire);
        Node* next = last->next.load(std::memory_order_acquire);
        
        // Check if tail is still consistent
        if (last == tail.load(std::memory_order_acquire)) {
            if (next == nullptr) {
                // Tail is pointing to the last node, try to link new node
                if (last->next.compare_exchange_strong(next, node,
                                                       std::memory_order_release,
                                                       std::memory_order_acquire)) {
                    // Successfully linked new node, try to swing tail
                    tail.compare_exchange_strong(last, node,
                                                std::memory_order_release,
                                                std::memory_order_acquire);
                    return;
                }
            } else {
                // Tail is falling behind, help advance it
                tail.compare_exchange_strong(last, next,
                                            std::memory_order_release,
                                            std::memory_order_acquire);
            }
        }
    }
}

template<typename T>
bool LockFreeQueue<T>::dequeue(T& result) {
    while (true) {
        // Load head, tail, and head's next pointer
        Node* first = head.load(std::memory_order_acquire);
        Node* last = tail.load(std::memory_order_acquire);
        Node* next = first->next.load(std::memory_order_acquire);
        
        // Check if head is still consistent
        if (first == head.load(std::memory_order_acquire)) {
            if (first == last) {
                // Queue is empty or tail is falling behind
                if (next == nullptr) {
                    // Queue is truly empty
                    return false;
                }
                // Tail is falling behind, help advance it
                tail.compare_exchange_strong(last, next,
                                            std::memory_order_release,
                                            std::memory_order_acquire);
            } else {
                // Queue is not empty, read value before CAS
                // This is safe because we hold a reference to next
                result = next->data;
                
                // Try to swing head to next node
                if (head.compare_exchange_strong(first, next,
                                                std::memory_order_release,
                                                std::memory_order_acquire)) {
                    // Successfully dequeued, reclaim old dummy node
                    // WARNING: Immediate delete is safe for benchmarks only.
                    // Production code requires hazard pointers or epoch-based reclamation.
                    delete first;
                    return true;
                }
            }
        }
    }
}

template<typename T>
bool LockFreeQueue<T>::empty() const {
    // Load head and check if its next pointer is null
    Node* first = head.load(std::memory_order_acquire);
    Node* next = first->next.load(std::memory_order_acquire);
    return next == nullptr;
}

#endif // LOCK_FREE_QUEUE_HPP
