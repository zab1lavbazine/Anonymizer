cmake_minimum_required(VERSION 3.10)
project(Anonymizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(CAPNP_DIR ${CMAKE_SOURCE_DIR}/capnproto)

# include directories
include_directories(${INCLUDE_DIR} ${CAPNP_DIR})

#Find packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)

#Compile capnproto file
find_program(CAPNP_COMPILER capnp)
find_program(CAPNP_CXX_COMPILER capnpc-c++)

if (NOT CAPNP_COMPILER OR NOT CAPNP_CXX_COMPILER )
    message(FATAL_ERROR "Capnp Proto compiler (capnp) or C++ plugin (capnpc-c++) not found")
endif()


file(GLOB CAPNP_FILES ${CAPNP_DIR}/*.capnp)
set(GENERATED_CAPNP_FILES "")

foreach(capnp_file ${CAPNP_FILES})
    get_filename_component(name_we ${capnp_file} NAME_WE)
    set(output_cpp ${CMAKE_CURRENT_BINARY_DIR}/${name_we}.capnp.c++)
    set(output_h ${CMAKE_CURRENT_BINARY_DIR}/${name_we}.capnp.h)

    add_custom_command(
        OUTPUT ${output_cpp} ${output_h}
        COMMAND ${CAPNP_COMPILER} compile -oc++:${CMAKE_CURRENT_BINARY_DIR} ${capnp_file}
        DEPENDS ${capnp_file}
        COMMENT "Compiling Cap’n Proto schema ${capnp_file}"
        VERBATIM
    )
    
    list(APPEND GENERATED_CAPNP_FILES ${output_cpp})
endforeach()

# Ensure Cap’n Proto files are built before compiling the main program
add_custom_target(capnp_generate DEPENDS ${GENERATED_CAPNP_FILES})

# Source files
file(GLOB SRC_FILES ${SRC_DIR}/*.cpp)
set(SOURCES ${SRC_FILES} ${GENERATED_CAPNP_FILES})

# Create executable
add_executable(anonymizer ${SOURCES})

# Ensure Cap’n Proto is generated before building
add_dependencies(anonymizer capnp_generate)

# Link libraries
target_link_libraries(anonymizer
    capnp capnp-rpc kj
    rdkafka rdkafka++ ssl crypto
    cpprest Boost::system Boost::thread pthread
)

# Ensure the build directory is included for generated Cap’n Proto headers
target_include_directories(anonymizer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})