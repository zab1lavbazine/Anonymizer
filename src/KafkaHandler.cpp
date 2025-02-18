#include "KafkaHandler.hpp"

KafkaHandler::KafkaHandler()
    : kafkaConsumer(nullptr),
      httpSender(nullptr),
      kafkaConsumerThread(nullptr),
      httpSenderThread(nullptr) {}

KafkaHandler::~KafkaHandler() {
  if (kafkaConsumerThread) {
    kafkaConsumerThread->join();
  }
  if (httpSenderThread) {
    httpSenderThread->join();
  }
}

/// @brief start KafkaConsumer and HttpSender
void KafkaHandler::start() {
  if (kafkaConsumer) {
    kafkaConsumerThread = std::make_unique<std::thread>(
        &KafkaConsumer::startConsuming, kafkaConsumer.get());
    std::cout << "KafkaConsumer started" << std::endl;
  }

  if (httpSender) {
    httpSenderThread = std::make_unique<std::thread>(&HttpSender::startSending,
                                                     httpSender.get());
    std::cout << "HttpSender started" << std::endl;
  }
}

/// @brief  configure Kafka consumer
/// @param broker
/// @param topic
void KafkaHandler::configureKafkaConsumer(const std::string& broker,
                                          const std::string& topic) {
  kafkaConsumer = std::make_unique<KafkaConsumer>(broker, topic, &httpLogQueue);
  kafkaConsumer->configure();
}

/// @brief configure HttpSender
/// @param url
void KafkaHandler::configureHttpSender(const std::string& url) {
  httpSender = std::make_unique<HttpSender>(url, &httpLogQueue);
}
