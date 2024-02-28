# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Libraries
LIBS = -lcapnp -lkj -lcapnpc -lcpprest -lrdkafka++ -lrdkafka

# Directories
SRC_DIR = src
OBJ_DIR = obj

# Source files
SRCS = $(SRC_DIR)/server.cpp

# Object files
OBJS = $(OBJ_DIR)/http_log.capnp.o $(OBJ_DIR)/server.o

# Executable name
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Rule to compile capnp file
$(OBJ_DIR)/http_log.capnp.c++: $(SRC_DIR)/http_log.capnp
    capnp compile -oc++ $< -o $@

# Rule to compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
    $(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
    ./$(TARGET)

clean:
    rm -f $(TARGET) $(OBJS)

.PHONY: all run clean
