#ifndef HTTPLOG_HPP
#define HTTPLOG_HPP

#include <capnp/message.h>
#include <capnp/schema-parser.h>
#include <capnp/serialize.h>
#include <cpprest/http_client.h>
#include <kj/io.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "../build/home/bareldan/Anonymizer/capnproto/http_log.capnp.h"

#pragma once

/// @brief class HttpLog to store logs

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
  HttpLog();
  ~HttpLog();

  HttpLog(const HttpLogRecord::Reader& httpLogRecord);

  void setTimestampEpochMilli(int64_t timestampEpochMilli);
  void setResourceId(std::string resourceId);
  void setBytesSent(int64_t bytesSent);
  void setRequestTimeMilli(int64_t requestTimeMilli);
  void setResponseStatus(int32_t responseStatus);
  void setCacheStatus(std::string cacheStatus);
  void setMethod(std::string method);
  void setRemoteAddr(std::string remoteAddr);
  void setUrl(std::string url);

  int64_t getTimestampEpochMilli() const;
  std::string getResourceId() const;
  int64_t getBytesSent() const;
  int64_t getRequestTimeMilli() const;
  int32_t getResponseStatus() const;
  std::string getCacheStatus() const;
  std::string getMethod() const;
  std::string getRemoteAddr() const;
  std::string getUrl() const;

  std::string toSqlInsert() const;

  void anonymize();

  friend std::ostream& operator<<(std::ostream& os, const HttpLog& httpLog);
};

#endif  // HTTPLOG_HPP
