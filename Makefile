# Compiler options
CXX = g++
CPPFLAGS = -Isrc -I/usr/local/include
CXXFLAGS = -std=c++17 -Wall

# List of source files
SRCS = $(wildcard src/*.cpp)
CAPNP_FILE = $(wildcard src/*.capnp)
CAPNP_SRC = $(addprefix compiled/,$(notdir $(CAPNP_FILE:.capnp=.capnp.c++)))
CAPNP_HEADER = $(addprefix compiled/,$(notdir $(CAPNP_FILE:.capnp=.capnp.h)))

# Main target
TARGET = anonymizer

.PHONY: all clean

# Default target
all: $(TARGET)

# Create the compiled directory if it doesn't exist
$(shell mkdir -p compiled)

# Create the output directory if it doesn't exist
$(shell mkdir -p output)

# Rule to compile Cap'n Proto files
compiled/%.capnp.c++ compiled/%.capnp.h: src/%.capnp | compiled
	@mkdir -p compiled && cd src && capnp compile -oc++ --src-prefix=../src $(notdir $<)
	@mv src/*.capnp.* compiled/

# Rule to compile C++ source files
compiled/%.o: src/%.cpp $(CAPNP_HEADER)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ -Icompiled

# Linking rule
$(TARGET): $(SRCS:src/%.cpp=compiled/%.o)
	@$(CXX) $^ -o compiled/$@ \
	$(CAPNP_SRC) \
	-lcapnp -lcapnp-rpc -lkj \
	-lrdkafka -lrdkafka++ -lssl -lcrypto \
	-lcpprest -lboost_system -lboost_thread -lpthread

# Run rule
run: $(TARGET)
	@./compiled/$(TARGET)

# Clean rule
clean:
	@rm -rf $(TARGET) compiled output
