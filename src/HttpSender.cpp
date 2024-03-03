#include "HttpSender.hpp"

HttpSender::HttpSender(const std::string& url,
                       ThreadSafeQueue<HttpLog>* httpLogQueue,
                       std::mutex* mutex, std::condition_variable* condition)
    : url(url),
      httpLogQueue(httpLogQueue),
      mutex(mutex),
      condition(condition) {}

HttpSender::~HttpSender() {}

std::string HttpSender::constructSqlInsertQueries(
    const std::vector<HttpLog>& logs) {
  std::ostringstream requestBodyStream;

  OutputHandler::sendSize(logs.size());

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

void HttpSender::handleResponse(const web::http::http_response& response) {
  if (response.status_code() == web::http::status_codes::OK) {
    std::cout << "Request sent successfully" << std::endl;
    innerHttpLogVector.clear();
  } else {
    std::cerr << "Failed to send request. Status code: "
              << response.status_code() << std::endl;
    // Log error message
    OutputHandler::saveError("Failed to send request. Status code: " +
                                 std::to_string(response.status_code()),
                             LOG_ERROR_FILE);
  }
}

void HttpSender::handleRequestError(const std::string& errorMessage) {
  std::cerr << "Error during HTTP request : " << errorMessage << std::endl;
  OutputHandler::saveError("Error during HTTP request: " + errorMessage,
                           LOG_ERROR_FILE);
}

void HttpSender::send() {
  try {
    web::http::client::http_client client(U(url));
    web::http::http_request request(web::http::methods::POST);

    std::ostringstream requestBodyStream;
    requestBodyStream << constructSqlInsertQueries(innerHttpLogVector);

    request.headers().set_content_type(U("text/plain; charset=utf-8"));
    request.set_body(requestBodyStream.str());

    auto response = client.request(request).get();

    handleResponse(response);

    OutputHandler::saveError(requestBodyStream.str(), REQUEST_FILE);
    OutputHandler::saveError(response.to_string(), RESPONSE_FILE);
  } catch (const std::exception& ex) {
    handleRequestError(ex.what());
  }
}

void HttpSender::checkIfAvailable() {
  std::unique_lock<std::mutex> lock(*mutex);

  while (!httpLogQueue->empty()) {
    innerHttpLogVector.push_back(httpLogQueue->pop());
  }

  lock.unlock();
}

void HttpSender::createTable() {
  // Send queries to ClickHouse to create tables
  if (sendQueryToClickHouse(CREATE_TABLE_HTTP_LOG) &&
      sendQueryToClickHouse(CREATE_TABLE_HTTP_TRAFFIC_TOTALS_MV) &&
      sendQueryToClickHouse(CREATE_MV_TRAFFIC_TOTALS(
          http_traffic_totals, SummingMergeTree,
          (resource_id, http_status, cache_status, ip_address), http_log))) {
    tableCreated = true;
  }
}

bool HttpSender::sendQueryToClickHouse(const std::string& query) {
  web::http::client::http_client client(U(CLICKHOUSE_HOST));
  web::http::http_request request(web::http::methods::POST);
  request.headers().set_content_type(U("text/plain; charset=utf-8"));
  request.set_body(query);

  auto response = client.request(request).get();

  if (response.status_code() == web::http::status_codes::OK) {
    std::cout << "Query sent successfully" << std::endl;
    return true;
  } else {
    std::cerr << "Failed to send query. Status code: " << response.status_code()
              << std::endl;
    return false;
  }
}

void HttpSender::startSending() {
  while (true) {
    std::cout << "Sleeping for 1 minute... -------------------->>>\n";
    std::this_thread::sleep_for(std::chrono::minutes(1) +
                                std::chrono::seconds(5));
    // check the queue
    if (!tableCreated) {
      createTable();
    }
    checkIfAvailable();
    if (!innerHttpLogVector.empty() && tableCreated) {
      send();
    }
  }
}
