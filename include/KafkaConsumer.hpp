#ifndef KAFKA_CONSUMER_HPP
#define KAFKA_CONSUMER_HPP


#include <librdkafka/rdkafkacpp.h>

#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#include "CapncapDecoder.hpp"
#include "HttpLog.hpp"
#include "OutputHandler.hpp"
#include "ThreadSafeQueue.hpp"

#pragma once

class KafkaConsumer : public RdKafka::ConsumeCb {
 public:
  KafkaConsumer(const std::string& brokers, const std::string& topic,
                ThreadSafeQueue<HttpLog>* httpLogQueue, std::mutex* mutex,
                std::condition_variable* condition);
  ~KafkaConsumer();

  void configure();
  void startConsuming();

 private:
  void consume_cb(RdKafka::Message& message, void* opaquem);
  void processMessagePayload(const char* payload, size_t len);
  void pushInQueueIfAvailable(const HttpLog& httpLog);

  std::string brokers;
  std::string topic;
  std::string errstr;
  RdKafka::Conf* conf;
  RdKafka::Conf* tconf;
  RdKafka::KafkaConsumer* consumer;

  std::queue<HttpLog> innerHttpLogQueue;
  ThreadSafeQueue<HttpLog>* httpLogQueue;
  std::mutex* mutex;
  std::condition_variable* condition;
};

#endif  // KAFKA_CONSUMER_HPP
