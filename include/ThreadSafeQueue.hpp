
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#pragma once

/// @brief class ThreadSafeQueue to make a queue thread safe between
/// KafkaConsumer and HttpSender

template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() = default;

  bool tryPush(const T& value);

  std::optional<T> pop();

  bool empty() const;

  size_t size() const;

  void shutdown();

 private:
  std::queue<T> queue;
  mutable std::mutex mutex;
  std::condition_variable condition;
  std::atomic<bool> stop = false;
};