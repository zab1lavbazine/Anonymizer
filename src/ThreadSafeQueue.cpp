
#include "ThreadSafeQueue.hpp"

template <typename T>
bool ThreadSafeQueue<T>::tryPush(const T& value) {
  std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
  if (!lock) return false;  // Lock is busy, return immediately
  queue.push(value);
  condition.notify_one();
  return true;
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::pop() {
  std::unique_lock<std::mutex> lock(mutex);
  condition.wait(lock, [this] { return !queue.empty() || stop; });

  if (queue.empty()) return std::nullopt;  // returning nullptr

  T value = queue.front();
  queue.pop();
  return value;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const {
  std::lock_guard<std::mutex> lock(mutex);
  return queue.empty();
}

template <typename T>
size_t ThreadSafeQueue<T>::size() const {
  std::lock_guard<std::mutex> lock(mutex);
  return queue.size();
}

template <typename T>
void ThreadSafeQueue<T>::shutdown() {
  std::lock_guard<std::mutex> lock(mutex);
  stop = true;
  condition.notify_all();
}