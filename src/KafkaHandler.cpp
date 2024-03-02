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

void KafkaHandler::configureKafkaConsumer(const std::string& broker,
                                          const std::string& topic) {
  kafkaConsumer = std::make_unique<KafkaConsumer>(broker, topic, &httpLogQueue,
                                                  &mutex, &condition);
  kafkaConsumer->configure();
}

void KafkaHandler::configureHttpSender(const std::string& url) {
  httpSender =
      std::make_unique<HttpSender>(url, &httpLogQueue, &mutex, &condition);
}
