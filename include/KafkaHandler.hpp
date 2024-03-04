#ifndef KAFKA_HANDLER_HPP
#define KAFKA_HANDLER_HPP

#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#include "HttpLog.hpp"
#include "HttpSender.hpp"
#include "KafkaConsumer.hpp"
#include "ThreadSafeQueue.hpp"

#pragma once

/// @brief class KafkaHandler to handle Kafka and HTTP

class KafkaHandler {
 public:
  KafkaHandler();
  ~KafkaHandler();

  void start();
  void configureKafkaConsumer(const std::string& broker,
                              const std::string& topic);
  void configureHttpSender(const std::string& url);

 private:
  std::unique_ptr<KafkaConsumer> kafkaConsumer;
  std::unique_ptr<HttpSender> httpSender;
  std::unique_ptr<std::thread> kafkaConsumerThread;
  std::unique_ptr<std::thread> httpSenderThread;
  ThreadSafeQueue<HttpLog> httpLogQueue;
  std::mutex mutex;
  std::condition_variable condition;
};

#endif  // KAFKA_HANDLER_HPP
