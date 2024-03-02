
#ifndef OUTPUT_HANDLER_HPP
#define OUTPUT_HANDLER_HPP

#include <fstream>
#include <iostream>
#include <string>

#pragma once

class OutputHandler {
 public:
  static void sendSize(size_t size);
  static void sendErrorInFile(const std::string& error);
  static void sendRequestInFile(const std::string& requestBody);
  static void sendResponseInFile(const std::string& response);
};

#endif  // OUTPUT_HANDLER_HPP
