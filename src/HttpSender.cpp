#include "HttpSender.hpp"

HttpSender::HttpSender(const std::string& url,
                       ThreadSafeQueue<HttpLog>* httpLogQueue,
                       std::mutex* mutex, std::condition_variable* condition)
    : url(url),
      httpLogQueue(httpLogQueue),
      mutex(mutex),
      condition(condition) {}

HttpSender::~HttpSender() {}

/// @brief Construct SQL INSERT queries for each log entry

std::string HttpSender::constructSqlInsertQueries(
    const std::vector<HttpLog>& logs) {
  std::ostringstream requestBodyStream;

  OutputHandler::saveError(std::to_string(logs.size()), SIZE_FILE);

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

/// @brief Handle the response from the server

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

/// @brief Handle the error during the request
void HttpSender::handleRequestError(const std::string& errorMessage) {
  std::cerr << "Error during HTTP request : " << errorMessage << std::endl;
  OutputHandler::saveError("Error during HTTP request: " + errorMessage,
                           LOG_ERROR_FILE);
}

/// @brief  Send the logs to the chproxy
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

/// @brief Check if there are logs available to send from the safe queue
void HttpSender::checkIfAvailable() {
  std::unique_lock<std::mutex> lock(*mutex);

  while (!httpLogQueue->empty()) {
    innerHttpLogVector.push_back(httpLogQueue->pop());
  }

  lock.unlock();
}

/// @brief Create the table in ClickHouse if it does not exist
void HttpSender::createTable() {
  // Send queries to ClickHouse to create tables
  if (sendQueryToClickHouse(CREATE_TABLE_HTTP_LOG) &&
      sendQueryToClickHouse(CREATE_TABLE_HTTP_TRAFFIC_TOTALS_MV)) {
    tableCreated = true;
  }
}

bool HttpSender::handleResponseClickhouse(
    const web::http::http_response& response) {
  if (response.status_code() == web::http::status_codes::OK) {
    std::cout << "Request sent successfully" << std::endl;
    return true;
  } else {
    std::cerr << "Failed to send request. Status code: "
              << response.status_code() << std::endl;
    // Log error message
    OutputHandler::saveError("Failed to send request. Status code: " +
                                 std::to_string(response.status_code()),
                             LOG_ERROR_FILE);
    return false;
  }
}

bool HttpSender::sendQueryToClickHouse(const std::string& query) {
  try {
    web::http::client::http_client client(U(CLICKHOUSE_HOST));
    web::http::http_request request(web::http::methods::POST);
    request.headers().set_content_type(U("text/plain; charset=utf-8"));
    request.set_body(query);

    auto response = client.request(request).get();

    return handleResponseClickhouse(response);

  } catch (const std::exception& ex) {
    handleRequestError(ex.what());
  }

  return false;
}

/// @brief Start method to start sending logs to ClickHouse
void HttpSender::startSending() {
  while (true) {
    std::cout << "Sleeping for 1 minute... -------------------->>>\n";
    std::this_thread::sleep_for(std::chrono::minutes(1) +
                                std::chrono::seconds(5));
    // check the table creation
    if (!tableCreated) {
      std::cout << "Creating table... -------------------->>>\n";
      createTable();
    }

    checkIfAvailable();
    if (!innerHttpLogVector.empty() && tableCreated) {
      std::cout << "Sending... -------------------->>>\n";
      send();
    }
  }
}
