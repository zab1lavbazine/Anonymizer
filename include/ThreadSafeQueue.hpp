
#include <condition_variable>
#include <mutex>
#include <queue>

#pragma once

template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() = default;

  void push(const T& value) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(value);
    condition.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [this] { return !queue.empty(); });
    T value = queue.front();
    queue.pop();
    return value;
  }
  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
  }

 private:
  std::queue<T> queue;
  mutable std::mutex mutex;
  std::condition_variable condition;
};