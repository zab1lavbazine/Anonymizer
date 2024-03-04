
#ifndef OUTPUT_HANDLER_HPP
#define OUTPUT_HANDLER_HPP

#include <fstream>
#include <iostream>
#include <string>

#define OUTPUT_DIR "./output/"

#define SIZE_FILE OUTPUT_DIR "size.txt"
#define ERROR_FILE OUTPUT_DIR "error.txt"
#define REQUEST_FILE OUTPUT_DIR "request.txt"
#define RESPONSE_FILE OUTPUT_DIR "response.txt"
#define LOG_ERROR_FILE OUTPUT_DIR "logError.txt"

#pragma once

/// @brief  Class to handle output
class OutputHandler {
 public:
  static void saveError(const std::string& error, const std::string& filename);
};

#endif  // OUTPUT_HANDLER_HPP
