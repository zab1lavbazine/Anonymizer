#include "KafkaConsumer.hpp"

#pragma once

KafkaConsumer::KafkaConsumer(const std::string& brokers,
                             const std::string& topic,
                             ThreadSafeQueue<HttpLog>* httpLogQueue,
                             std::mutex* mutex,
                             std::condition_variable* condition)
    : brokers(brokers),
      topic(topic),
      httpLogQueue(httpLogQueue),
      mutex(mutex),
      condition(condition) {
  std::string errstr;
  conf = nullptr;
  tconf = nullptr;
  consumer = nullptr;
}

KafkaConsumer::~KafkaConsumer() {
  if (consumer) {
    consumer->close();
    delete consumer;
  }
  if (conf) {
    delete conf;
  }
  if (tconf) {
    delete tconf;
  }
}

void KafkaConsumer::configure() {
  conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

  conf->set("metadata.broker.list", brokers, errstr);
  conf->set("group.id", "http_log_consumer", errstr);

  // Create the Kafka consumer
  consumer = RdKafka::KafkaConsumer::create(conf, errstr);
  if (!consumer) {
    std::cerr << "Failed to create Kafka consumer: " << errstr << std::endl;
    delete conf;
    delete tconf;
    exit(EXIT_FAILURE);
  }

  // Subscribe to the topic
  RdKafka::ErrorCode resp = consumer->subscribe({topic});
  if (resp != RdKafka::ERR_NO_ERROR) {
    std::cerr << "Failed to subscribe to topic: " << RdKafka::err2str(resp)
              << std::endl;
    consumer->close();
    delete consumer;
    delete tconf;
    delete conf;
    exit(EXIT_FAILURE);
  }

  std::cout << "Kafka consumer subscribed to " << topic << std::endl;
}

void KafkaConsumer::startConsuming() {
  while (true) {
    RdKafka::Message* message = consumer->consume(1000);
    consume_cb(*message, nullptr);
    delete message;
  }
}

void KafkaConsumer::processMessagePayload(const char* payload, size_t len) {
  if (payload && len > 0) {
    std::string alignedBuffer(payload, len);
    HttpLog httpLog =
        CapncapDecoder::decodeMessagePayload(alignedBuffer.c_str(), len);
    pushInQueueIfAvailable(httpLog);
  }
}

void KafkaConsumer::consume_cb(RdKafka::Message& message, void* opaquem) {
  switch (message.err()) {
    case RdKafka::ERR__TIMED_OUT:
      break;

    case RdKafka::ERR_NO_ERROR:
      processMessagePayload(static_cast<const char*>(message.payload()),
                            message.len());
      break;

    default:
      std::cerr << "ERROR: " << message.errstr() << std::endl;
      OutputHandler::sendErrorInFile(message.errstr());
      handleKafkaError(message.err());
      break;
  }
}

void KafkaConsumer::pushInQueueIfAvailable(const HttpLog& httpLog) {
  if (mutex->try_lock()) {
    if (httpLogQueue->size() < 1000) {
      while (!innerHttpLogQueue.empty()) {
        httpLogQueue->push(innerHttpLogQueue.front());
        innerHttpLogQueue.pop();
      }
      httpLogQueue->push(httpLog);

      condition->notify_one();
    }
    mutex->unlock();
  } else {
    innerHttpLogQueue.push(httpLog);
  }
}

void KafkaConsumer::handleKafkaError(RdKafka::ErrorCode errorCode) {
  switch (errorCode) {
    case RdKafka::ERR__TRANSPORT:
      std::cerr << "Transport error occurred. Retrying operation..."
                << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      startConsuming();
      break;
    case RdKafka::ERR__ALL_BROKERS_DOWN:
      std::cerr << "All Kafka brokers are down. Exiting application."
                << std::endl;
      exit(EXIT_FAILURE);
      break;
    default:
      std::cerr << "Unhandled Kafka error: " << RdKafka::err2str(errorCode)
                << std::endl;
      break;
  }
}