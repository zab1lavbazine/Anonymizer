

#include "OutputHandler.hpp"

void OutputHandler::saveError(const std::string& error,
                              const std::string& filename) {
  std::ofstream outputFile(filename.c_str(), std::ofstream::out);
  if (outputFile.is_open()) {
    outputFile << error;
    outputFile.close();
    std::cout << "Error saved to file '" << filename << "'" << std::endl;
  } else {
    std::cerr << "Failed to open file '" << filename << "' for writing"
              << std::endl;
  }
}