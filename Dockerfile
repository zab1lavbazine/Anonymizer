# Use a base image with a C++ compiler
FROM gcc:latest

# Install system dependencies
RUN apt-get update && apt-get install -y \
    libcapnp-dev \
    libcpprest-dev \
    librdkafka-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /app

# Copy the source code into the container
COPY server.cpp .
COPY http_log.capnp.h .

# Compile the source code
RUN g++ -o server server.cpp -lcapnp -lkj -lcapnpc -lcpprest -lrdkafka++ -lrdkafka

# Set the entry point for the container
CMD ["./server"]
