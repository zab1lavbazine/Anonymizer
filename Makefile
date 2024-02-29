# Compiler options
CXX = g++
CPPFLAGS = -Isrc -I/usr/local/include
CXXFLAGS = -std=c++17 -Wall

# List of source files
SRCS = $(wildcard src/*.cpp) $(wildcard src/*.capnp.c++)
CAPNP_FILE = $(wildcard src/*.capnp)

# Main target
TARGET = anonymizer

.PHONY: all clean

# Default target
all: $(TARGET)

# Rule to compile Cap'n Proto files
$(SRCS): $(CAPNP_FILE)
	cd src && capnp compile -oc++ $(notdir $(CAPNP_FILE))

# Linking rule
$(TARGET): $(SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ \
	-lcapnp -lcapnp-rpc -lkj \
	-lrdkafka -lrdkafka++ -lssl -lcrypto \
	-lcpprest -lboost_system -lboost_thread -lpthread

# Run rule
run: $(TARGET)
	./$(TARGET)

# Clean rule
clean:
	rm -f $(TARGET) $(wildcard src/*.capnp.c++)
