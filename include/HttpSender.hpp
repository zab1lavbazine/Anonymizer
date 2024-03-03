#ifndef HTTP_SENDER_HPP
#define HTTP_SENDER_HPP

#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#include "HttpLog.hpp"
#include "HttpLogQueries.hpp"
#include "OutputHandler.hpp"
#include "ThreadSafeQueue.hpp"
#pragma once

class HttpSender {
 public:
  HttpSender(const std::string& url, ThreadSafeQueue<HttpLog>* httpLogQueue,
             std::mutex* mutex, std::condition_variable* condition);
  ~HttpSender();

  void send();
  void checkIfAvailable();
  void createTable();
  bool sendQueryToClickHouse(const std::string& query);
  void startSending();
  void handleResponse(const web::http::http_response& response);
  void handleRequestError(const std::string& errorMessage);
  bool handleResponseClickhouse(const web::http::http_response& response);

 private:
  std::string constructSqlInsertQueries(const std::vector<HttpLog>& logs);

  std::string url;
  ThreadSafeQueue<HttpLog>* httpLogQueue;
  std::mutex* mutex;
  std::condition_variable* condition;
  std::vector<HttpLog> innerHttpLogVector;
  bool tableCreated = false;
};

#endif  // HTTP_SENDER_HPP
