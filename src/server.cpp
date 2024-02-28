#include <capnp/message.h>
#include <capnp/schema-parser.h>
#include <capnp/serialize.h>
#include <cpprest/http_client.h>
#include <kj/io.h>
#include <librdkafka/rdkafkacpp.h>

#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "http_log.capnp.h"

void sendSize(size_t size) {
  // open new file
  std::ofstream outputFile("size.txt", std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << size;
    outputFile.close();
    std::cout << "Size saved to file 'size.txt'" << std::endl;
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
  std::ofstream outputFile("request.txt", std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << requestBody;
    outputFile.close();
    std::cout << "Request saved to file 'request.txt'" << std::endl;
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
  KafkaHandler() {
    kafkaConsumer = nullptr;
    httpSender = nullptr;
    kafkaConsumerThread = nullptr;
    httpSenderThread = nullptr;
  }

  void start() {
    if (kafkaConsumer) {
      kafkaConsumerThread =
          new std::thread(&KafkaConsumer::startConsuming, kafkaConsumer);
      std::cout << "KafkaConsumer started" << std::endl;
    }

    if (httpSender) {
      httpSenderThread = new std::thread(&HttpSender::startSending, httpSender);
      std::cout << "HttpSender started" << std::endl;
    }
  }

  ~KafkaHandler() {
    if (kafkaConsumerThread && kafkaConsumerThread->joinable()) {
      kafkaConsumerThread->join();
    }
    if (httpSenderThread && httpSenderThread->joinable()) {
      httpSenderThread->join();
    }

    if (kafkaConsumer) {
      delete kafkaConsumer;
    }
    if (httpSender) {
      delete httpSender;
    }
  }

  void configureKafkaConsumer(const std::string& broker,
                              const std::string& topic) {
    kafkaConsumer =
        new KafkaConsumer(broker, topic, &httpLogQueue, &mutex, &condition);
    try {
      kafkaConsumer->configure();
    } catch (const std::exception& e) {
      delete kafkaConsumer;
      delete httpSender;
      kafkaConsumer = nullptr;
      httpSender = nullptr;
      throw;
    }
  }
  void configureHttpSender(const std::string& url) {
    httpSender = new HttpSender(url, &httpLogQueue, &mutex, &condition);
    std::cout << "HttpSender configured" << std::endl;
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

      consumer = RdKafka::KafkaConsumer::create(conf, errstr);
      if (!consumer) {
        std::cerr << "Failed to create consumer: " << errstr << std::endl;
        return;
      }

      RdKafka::ErrorCode resp = consumer->subscribe({topic});
      if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to subscribe to " << topic << ": "
                  << RdKafka::err2str(resp) << std::endl;
        return;
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

    void consume_cb(RdKafka::Message& message, void* opaquem) {
      switch (message.err()) {
        case RdKafka::ERR__TIMED_OUT:
          std::cerr << "Timed out" << std::endl;
          break;

        case RdKafka::ERR_NO_ERROR:
          // Message received successfully
          std::cout << "Message payload: "
                    << std::string(static_cast<char*>(message.payload()),
                                   message.len())
                    << std::endl;
          if (message.payload() && message.len() > 0) {
            size_t size = message.len();
            void* alignedBuffer = malloc(size);
            if (alignedBuffer == nullptr) {
              std::cerr << "Failed to allocate memory" << std::endl;
              return;
            }
            memcpy(alignedBuffer, message.payload(), size);
            HttpLog httpLog = decodeMessagePayload(
                static_cast<const char*>(alignedBuffer), size);
            // check if the queue is available
            pushInQueueIfAvailable(httpLog);
            free(alignedBuffer);
          }
          break;

        default:
          // Handle other errors
          std::cerr << "Error: " << message.errstr() << std::endl;
          break;
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

    void send() {
      web::http::client::http_client client(U(url));
      web::http::http_request request(web::http::methods::POST);

      std::ostringstream requestBodyStream;

      // Construct SQL INSERT queries for each log entry
      requestBodyStream << "INSERT INTO http_log (timestamp, resource_id, "
                           "bytes_sent, request_time_milli, response_status, "
                           "cache_status, method, remote_addr, url) VALUES\n";
      for (const auto& log : innerHttpLogVector) {
        requestBodyStream << log.toSqlInsert();
        if (&log != &innerHttpLogVector.back()) {
          requestBodyStream << ",\n";
        } else {
          requestBodyStream << ";";
        }
      }

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

      // Save the response to a file
      std::ofstream outputFile("response.txt", std::ofstream::out);
      if (outputFile.is_open()) {
        outputFile << response.to_string();
        outputFile.close();
        std::cout << "Response saved to file 'response.txt'" << std::endl;
      } else {
        std::cerr << "Failed to open file for writing" << std::endl;
      }
    }

    void checkIfAvailable() {
      std::unique_lock<std::mutex> lock(*mutex);

      while (!httpLogQueue->empty()) {
        innerHttpLogVector.push_back(httpLogQueue->pop());
      }

      lock.unlock();

      if (!innerHttpLogVector.empty()) {
        sendSize(innerHttpLogVector.size());
        send();
      }
    }

    void startSending() {
      while (true) {
        std::cout << "Sleeping for 1 minute... -------------------->>>\n";
        std::this_thread::sleep_for(std::chrono::minutes(1));
        // check the queue
        checkIfAvailable();
      }
    }

   private:
    std::string url;
    ThreadSafeQueue<HttpLog>* httpLogQueue;
    std::mutex* mutex;
    std::condition_variable* condition;
    std::vector<HttpLog> innerHttpLogVector;
  };

 private:
  KafkaConsumer* kafkaConsumer;
  HttpSender* httpSender;
  std::thread* kafkaConsumerThread;
  std::thread* httpSenderThread;
  ThreadSafeQueue<HttpLog> httpLogQueue;
  std::mutex mutex;
  std::condition_variable condition;
};

int main(void) {
  KafkaHandler kafkaHandler;

  // commented for the docker
  kafkaHandler.configureKafkaConsumer("localhost:9092", "http_log");
  // kafkaHandler.configureKafkaConsumer("broker:9092", "http_log");
  kafkaHandler.configureHttpSender("http://localhost:8124/clickhouse-endpoint");
  // kafkaHandler.configureHttpSender("http://ch-proxy:8124/clickhouse-endpoint");

  kafkaHandler.start();

  return 0;
}