
#ifndef THREAD_SAFETY_HPP
#define THREAD_SAFETY_HPP

#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace bolt {

template<typename T>
class ThreadSafe {
private:
    mutable std::shared_mutex mutex_;
    T data_;

public:
    ThreadSafe(T data = T{}) : data_(std::move(data)) {}

    // Read operations
    template<typename F>
    auto read(F&& func) const {
        std::shared_lock lock(mutex_);
        return func(data_);
    }

    // Write operations
    template<typename F>
    auto write(F&& func) {
        std::unique_lock lock(mutex_);
        return func(data_);
    }

    // Atomic swap
    void swap(T& other) {
        std::unique_lock lock(mutex_);
        std::swap(data_, other);
    }
};

class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {}
    }

    void unlock() {
        flag_.clear(std::memory_order_release);
    }
};

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;
        Node() : next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~LockFreeQueue() {
        while (Node* const old = head_.load()) {
            head_.store(old->next);
            delete old;
        }
    }

    void push(T value) {
        Node* new_node = new Node;
        new_node->data = std::make_shared<T>(std::move(value));
        
        while (true) {
            Node* last = tail_.load();
            Node* next = last->next.load();
            
            if (last == tail_.load()) {
                if (next == nullptr) {
                    if (last->next.compare_exchange_strong(next, new_node)) {
                        tail_.compare_exchange_strong(last, new_node);
                        return;
                    }
                } else {
                    tail_.compare_exchange_strong(last, next);
                }
            }
        }
    }

    bool try_pop(T& value) {
        while (true) {
            Node* first = head_.load();
            Node* last = tail_.load();
            Node* next = first->next.load();
            
            if (first == head_.load()) {
                if (first == last) {
                    if (next == nullptr) {
                        return false;
                    }
                    tail_.compare_exchange_strong(last, next);
                } else {
                    if (next == nullptr) {
                        return false;
                    }
                    value = std::move(*next->data);
                    if (head_.compare_exchange_strong(first, next)) {
                        delete first;
                        return true;
                    }
                }
            }
        }
    }
};

} // namespace bolt

#endif
