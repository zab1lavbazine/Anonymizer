

#include "OutputHandler.hpp"

void OutputHandler::sendSize(size_t size) {
  std::ofstream outputFile("./output/size.txt", std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << size;
    outputFile.close();
    std::cout << "Size saved to file 'size.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}

void OutputHandler::sendErrorInFile(const std::string& error) {
  std::ofstream outputFile("./output/error.txt", std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << error;
    outputFile.close();
    std::cout << "Error saved to file 'error.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}

void OutputHandler::sendRequestInFile(const std::string& requestBody) {
  std::ofstream outputFile("./output/request.txt", std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << requestBody;
    outputFile.close();
    std::cout << "Request saved to file 'request.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}

void OutputHandler::sendResponseInFile(const std::string& response) {
  std::ofstream outputFile("./output/response.txt", std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << response;
    outputFile.close();
    std::cout << "Response saved to file 'response.txt'" << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }
}
