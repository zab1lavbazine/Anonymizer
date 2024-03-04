#include "HttpLog.hpp"

HttpLog::HttpLog()
    : timestampEpochMilli(0),
      resourceId(""),
      bytesSent(0),
      requestTimeMilli(0),
      responseStatus(0),
      cacheStatus(""),
      method(""),
      remoteAddr(""),
      url("") {}

HttpLog::~HttpLog() {}

HttpLog::HttpLog(const HttpLogRecord::Reader& httpLogRecord)
    : timestampEpochMilli(httpLogRecord.getTimestampEpochMilli()),
      resourceId(std::to_string(httpLogRecord.getResourceId())),
      bytesSent(httpLogRecord.getBytesSent()),
      requestTimeMilli(httpLogRecord.getRequestTimeMilli()),
      responseStatus(httpLogRecord.getResponseStatus()),
      cacheStatus(httpLogRecord.getCacheStatus().cStr()),
      method(httpLogRecord.getMethod().cStr()),
      remoteAddr(httpLogRecord.getRemoteAddr().cStr()),
      url(httpLogRecord.getUrl().cStr()) {}

void HttpLog::setTimestampEpochMilli(int64_t timestampEpochMilli) {
  this->timestampEpochMilli = timestampEpochMilli;
}

void HttpLog::setResourceId(std::string resourceId) {
  this->resourceId = resourceId;
}

void HttpLog::setBytesSent(int64_t bytesSent) { this->bytesSent = bytesSent; }

void HttpLog::setRequestTimeMilli(int64_t requestTimeMilli) {
  this->requestTimeMilli = requestTimeMilli;
}

void HttpLog::setResponseStatus(int32_t responseStatus) {
  this->responseStatus = responseStatus;
}

void HttpLog::setCacheStatus(std::string cacheStatus) {
  this->cacheStatus = cacheStatus;
}

void HttpLog::setMethod(std::string method) { this->method = method; }

void HttpLog::setRemoteAddr(std::string remoteAddr) {
  this->remoteAddr = remoteAddr;
}

void HttpLog::setUrl(std::string url) { this->url = url; }

int64_t HttpLog::getTimestampEpochMilli() const {
  return this->timestampEpochMilli;
}

std::string HttpLog::getResourceId() const { return this->resourceId; }

int64_t HttpLog::getBytesSent() const { return this->bytesSent; }

int64_t HttpLog::getRequestTimeMilli() const { return this->requestTimeMilli; }

int32_t HttpLog::getResponseStatus() const { return this->responseStatus; }

std::string HttpLog::getCacheStatus() const { return this->cacheStatus; }

std::string HttpLog::getMethod() const { return this->method; }

std::string HttpLog::getRemoteAddr() const { return this->remoteAddr; }

std::string HttpLog::getUrl() const { return this->url; }

std::string HttpLog::toSqlInsert() const {
  std::ostringstream sqlInsertStream;
  sqlInsertStream << "("
                  << "'" << timestampEpochMilli << "', "
                  << "'" << resourceId << "', " << bytesSent << ", "
                  << requestTimeMilli << ", " << responseStatus << ", "
                  << "'" << cacheStatus << "', "
                  << "'" << method << "', "
                  << "'" << remoteAddr << "', "
                  << "'" << url << "')";

  return sqlInsertStream.str();
}

/// @brief Anonymize the remoteAddr field by replacing the last octet with an X

void HttpLog::anonymize() {
  // Modify the remoteAddr field
  size_t lastDotIndex = this->remoteAddr.find_last_of('.');
  if (lastDotIndex != std::string::npos) {
    this->remoteAddr.replace(this->remoteAddr.begin() + lastDotIndex + 1,
                             this->remoteAddr.end(), "X");
  }
}

std::ostream& operator<<(std::ostream& os, const HttpLog& httpLog) {
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
