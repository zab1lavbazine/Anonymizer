

#include "KafkaHandler.hpp"

int main(void) {
  KafkaHandler kafkaHandler;

  try {
    kafkaHandler.configureKafkaConsumer("localhost:9092", "http_log");
    kafkaHandler.configureHttpSender("http://localhost:8124");
    kafkaHandler.start();
    return 0;
  } catch (const std::exception& e) {
    std::cout << "Error : " << e.what() << std::endl;
    return 1;
  }

  return 0;
}