#include <capnp/message.h>
#include <capnp/schema-parser.h>
#include <capnp/serialize.h>
#include <cpprest/http_client.h>
#include <kj/io.h>
#include <librdkafka/rdkafkacpp.h>

#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#define FILE_PATH "./output/"

#include "HttpLogQueries.hpp"
#include "http_log.capnp.h"

void sendSize(size_t size) {
  // open new file
  std::ofstream outputFile(std::string(FILE_PATH) + "size.txt",
                           std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << size;
    outputFile.close();
    std::cout << "Size saved to file 'size.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}
void sendErrorInFile(const std::string& error) {
  // open new file
  std::ofstream outputFile(std::string(FILE_PATH) + "error.txt",
                           std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << error;
    outputFile.close();
    std::cout << "Error saved to file 'error.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}

class HttpLog {
 private:
  int64_t timestampEpochMilli;
  std::string resourceId;
  int64_t bytesSent;
  int64_t requestTimeMilli;
  int32_t responseStatus;
  std::string cacheStatus;
  std::string method;
  std::string remoteAddr;
  std::string url;

 public:
  HttpLog() {}
  ~HttpLog() {}

  HttpLog(const HttpLogRecord::Reader& httpLogRecord)
      : timestampEpochMilli(httpLogRecord.getTimestampEpochMilli()),
        resourceId(std::to_string(httpLogRecord.getResourceId())),
        bytesSent(httpLogRecord.getBytesSent()),
        requestTimeMilli(httpLogRecord.getRequestTimeMilli()),
        responseStatus(httpLogRecord.getResponseStatus()),
        cacheStatus(httpLogRecord.getCacheStatus().cStr()),
        method(httpLogRecord.getMethod().cStr()),
        remoteAddr(httpLogRecord.getRemoteAddr().cStr()),
        url(httpLogRecord.getUrl().cStr()) {}

  void setTimestampEpochMilli(int64_t timestampEpochMilli) {
    this->timestampEpochMilli = timestampEpochMilli;
  }
  void setResourceId(std::string resourceId) { this->resourceId = resourceId; }
  void setBytesSent(int64_t bytesSent) { this->bytesSent = bytesSent; }
  void setRequestTimeMilli(int64_t requestTimeMilli) {
    this->requestTimeMilli = requestTimeMilli;
  }
  void setResponseStatus(int32_t responseStatus) {
    this->responseStatus = responseStatus;
  }
  void setCacheStatus(std::string cacheStatus) {
    this->cacheStatus = cacheStatus;
  }
  void setMethod(std::string method) { this->method = method; }
  void setRemoteAddr(std::string remoteAddr) { this->remoteAddr = remoteAddr; }
  void setUrl(std::string url) { this->url = url; }

  int64_t getTimestampEpochMilli() const { return this->timestampEpochMilli; }
  std::string getResourceId() const { return this->resourceId; }
  int64_t getBytesSent() const { return this->bytesSent; }
  int64_t getRequestTimeMilli() const { return this->requestTimeMilli; }
  int32_t getResponseStatus() const { return this->responseStatus; }
  std::string getCacheStatus() const { return this->cacheStatus; }
  std::string getMethod() const { return this->method; }
  std::string getRemoteAddr() const { return this->remoteAddr; }
  std::string getUrl() const { return this->url; }

  std::string toSqlInsert() const {
    std::ostringstream sqlInsertStream;
    sqlInsertStream << "("
                    << "'" << timestampEpochMilli << "', " << resourceId << ", "
                    << bytesSent << ", " << requestTimeMilli << ", "
                    << responseStatus << ", "
                    << "'" << cacheStatus << "', "
                    << "'" << method << "', "
                    << "'" << remoteAddr << "', "
                    << "'" << url << "')";

    return sqlInsertStream.str();
  }

  void anonymize() {
    // Modify the remoteAddr field
    size_t lastDotIndex = this->remoteAddr.find_last_of('.');
    if (lastDotIndex != std::string::npos) {
      this->remoteAddr.replace(this->remoteAddr.begin() + lastDotIndex + 1,
                               this->remoteAddr.end(), "X");
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const HttpLog& httpLog) {
    os << "timestampEpochMilli: " << httpLog.timestampEpochMilli << std::endl;
    os << "resourceId: " << httpLog.resourceId << std::endl;
    os << "bytesSent: " << httpLog.bytesSent << std::endl;
    os << "requestTimeMilli: " << httpLog.requestTimeMilli << std::endl;
    os << "responseStatus: " << httpLog.responseStatus << std::endl;
    os << "cacheStatus: " << httpLog.cacheStatus << std::endl;
    os << "method: " << httpLog.method << std::endl;
    os << "remoteAddr: " << httpLog.remoteAddr << std::endl;
    os << "url: " << httpLog.url << std::endl;
    return os;
  }
};

void sendRequestInFile(const std::string& requestBody) {
  // open new file
  std::ofstream outputFile(std::string(FILE_PATH) + "request.txt",
                           std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << requestBody;
    outputFile.close();
    std::cout << "Request saved to file 'request.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}

void saveResponseInFile(const std::string& response) {
  std::ofstream outputFile(std::string(FILE_PATH) + "response.txt",
                           std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << response;
    outputFile.close();
    std::cout << "Response saved to file 'response.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}

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

class KafkaHandler {
 public:
  KafkaHandler()
      : kafkaConsumer(nullptr),
        httpSender(nullptr),
        kafkaConsumerThread(nullptr),
        httpSenderThread(nullptr) {}

  void start() {
    if (kafkaConsumer) {
      kafkaConsumerThread = std::make_unique<std::thread>(
          &KafkaConsumer::startConsuming, kafkaConsumer.get());
      std::cout << "KafkaConsumer started" << std::endl;
    }

    if (httpSender) {
      httpSenderThread = std::make_unique<std::thread>(
          &HttpSender::startSending, httpSender.get());
      std::cout << "HttpSender started" << std::endl;
    }
  }

  ~KafkaHandler() {
    if (kafkaConsumerThread) {
      kafkaConsumerThread->join();
    }
    if (httpSenderThread) {
      httpSenderThread->join();
    }
  }

  void configureKafkaConsumer(const std::string& broker,
                              const std::string& topic) {
    kafkaConsumer = std::make_unique<KafkaConsumer>(
        broker, topic, &httpLogQueue, &mutex, &condition);
    kafkaConsumer->configure();
  }

  void configureHttpSender(const std::string& url) {
    httpSender =
        std::make_unique<HttpSender>(url, &httpLogQueue, &mutex, &condition);
  }

 private:
  class KafkaConsumer : public RdKafka::ConsumeCb {
   public:
    KafkaConsumer(const std::string& brokers, const std::string& topic,
                  ThreadSafeQueue<HttpLog>* httpLogQueue, std::mutex* mutex,
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

    ~KafkaConsumer() {
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

    void configure() {
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

      // // Subscribe to the topic
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

    void startConsuming() {
      while (true) {
        RdKafka::Message* message = consumer->consume(1000);
        consume_cb(*message, nullptr);
        delete message;
      }
    }

    void processMessagePayload(const char* payload, size_t len) {
      if (payload && len > 0) {
        std::string alignedBuffer(payload, len);
        HttpLog httpLog = decodeMessagePayload(alignedBuffer.c_str(), len);
        pushInQueueIfAvailable(httpLog);
      }
    }
    void consume_cb(RdKafka::Message& message, void* opaquem) {
      switch (message.err()) {
        case RdKafka::ERR__TIMED_OUT:
          break;

        case RdKafka::ERR_NO_ERROR:
          processMessagePayload(static_cast<const char*>(message.payload()),
                                message.len());
          break;

        default:
          // Handle other errors
          std::cerr << "Error: " << message.errstr() << std::endl;
          sendErrorInFile(message.errstr());
          consumer->close();
          exit(EXIT_FAILURE);
      }
    }

   private:
    /// @brief Kafka consumer configuration
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

    ///----------------------------------------------------------------------------

    HttpLog decodeMessagePayload(const char* decodeMessagePayload,
                                 size_t size) {
      kj::ArrayPtr<const capnp::word> array(
          reinterpret_cast<const capnp::word*>(decodeMessagePayload),
          size / sizeof(capnp::word));
      capnp::FlatArrayMessageReader reader(array);
      HttpLogRecord::Reader httpLogRecord = reader.getRoot<HttpLogRecord>();

      HttpLog httpLog(httpLogRecord);
      httpLog.anonymize();

      std::cout << httpLog << std::endl;

      return httpLog;
    }

    void pushInQueueIfAvailable(const HttpLog& httpLog) {
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
  };

  class HttpSender {
   public:
    HttpSender(const std::string& url, ThreadSafeQueue<HttpLog>* httpLogQueue,
               std::mutex* mutex, std::condition_variable* condition)
        : url(url),
          httpLogQueue(httpLogQueue),
          mutex(mutex),
          condition(condition) {}
    ~HttpSender() {}

    std::string constructSqlInsertQueries(const std::vector<HttpLog>& logs) {
      std::ostringstream requestBodyStream;

      sendSize(logs.size());

      // Construct SQL INSERT queries for each log entry
      requestBodyStream << INSERT_INTO_DATABASE_HTTP_LOG << " ";
      for (auto it = logs.begin(); it != logs.end(); ++it) {
        requestBodyStream << it->toSqlInsert();
        if (std::next(it) != logs.end()) {
          requestBodyStream << ",\n";
        } else {
          requestBodyStream << ";";
        }
      }

      return requestBodyStream.str();
    }

    void send() {
      web::http::client::http_client client(U(url));
      web::http::http_request request(web::http::methods::POST);

      std::ostringstream requestBodyStream;
      requestBodyStream << constructSqlInsertQueries(innerHttpLogVector);

      request.headers().set_content_type(U("text/plain; charset=utf-8"));
      request.set_body(requestBodyStream.str());

      auto response = client.request(request).get();

      if (response.status_code() == web::http::status_codes::OK) {
        std::cout << "Request sent successfully" << std::endl;
        innerHttpLogVector.clear();
      } else {
        std::cerr << "Failed to send request. Status code: "
                  << response.status_code() << std::endl;
      }

      sendRequestInFile(requestBodyStream.str());
      saveResponseInFile(response.to_string());
    }

    void checkIfAvailable() {
      std::unique_lock<std::mutex> lock(*mutex);

      while (!httpLogQueue->empty()) {
        innerHttpLogVector.push_back(httpLogQueue->pop());
      }

      lock.unlock();

      send();
    }

    void createTable() {
      // Send queries to ClickHouse to create tables
      if (sendQueryToClickHouse(CREATE_TABLE_HTTP_LOG) &&
          sendQueryToClickHouse(CREATE_TABLE_HTTP_TRAFFIC_TOTALS_MV)) {
        tableCreated = true;
      }
    }
    bool sendQueryToClickHouse(const std::string& query) {
      web::http::client::http_client client(U("http://localhost:8123"));
      web::http::http_request request(web::http::methods::POST);
      request.headers().set_content_type(U("text/plain; charset=utf-8"));
      request.set_body(query);

      auto response = client.request(request).get();

      if (response.status_code() == web::http::status_codes::OK) {
        std::cout << "Query sent successfully" << std::endl;
        return true;
      } else {
        std::cerr << "Failed to send query. Status code: "
                  << response.status_code() << std::endl;
        return false;
      }
      return false;
    }

    void startSending() {
      while (true) {
        std::cout << "Sleeping for 1 minute... -------------------->>>\n";
        std::this_thread::sleep_for(std::chrono::minutes(1) +
                                    std::chrono::seconds(5));
        // check the queue
        if (!tableCreated) {
          createTable();
        }
        checkIfAvailable();
      }
    }

   private:
    std::string url;
    ThreadSafeQueue<HttpLog>* httpLogQueue;
    std::mutex* mutex;
    std::condition_variable* condition;
    std::vector<HttpLog> innerHttpLogVector;
    bool tableCreated = false;
  };

 private:
  std::unique_ptr<KafkaConsumer> kafkaConsumer;
  std::unique_ptr<HttpSender> httpSender;
  std::unique_ptr<std::thread> kafkaConsumerThread;
  std::unique_ptr<std::thread> httpSenderThread;
  ThreadSafeQueue<HttpLog> httpLogQueue;
  std::mutex mutex;
  std::condition_variable condition;
};

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