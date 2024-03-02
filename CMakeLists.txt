cmake_minimum_required(VERSION 3.10)
project(Anonymizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(include)
include_directories(compiled) # Add compiled directory to include Cap'n Proto headers
include_directories(/usr/local/include)

# Add executable target
add_executable(anonymizer 
    src/HttpLog.cpp 
    src/HttpSender.cpp 
    src/KafkaConsumer.cpp 
    src/KafkaHandler.cpp 
    src/OutputHandler.cpp 
    src/main.cpp 
    compiled/http_log.capnp.c++  # Add the compiled Cap'n Proto file
)

# Link libraries
target_link_libraries(anonymizer
    capnp capnp-rpc kj
    rdkafka rdkafka++ ssl crypto
    cpprest boost_system boost_thread pthread
)

# Define custom target for running the executable
add_custom_target(run
    COMMAND anonymizer
    DEPENDS anonymizer
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running the executable"
)
