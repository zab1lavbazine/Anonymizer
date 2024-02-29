# Use an appropriate base image
FROM ubuntu:latest

# Install required libraries
RUN apt-get update && apt-get install -y \
    g++ \
    libcapnp-dev \
    libcpprest-dev \
    librdkafka-dev \
    libssl-dev \
    libboost-system-dev \
    libboost-thread-dev \
    cmake

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY src/ /app/src/

# Compile the Cap'n Proto files
RUN apt-get install -y capnproto && \
    cd /app/src && \
    for file in *.capnp; do \
    capnp compile -oc++ "$file"; \
    done

# Compile the server.cpp file
RUN cd /app/src && \
    g++ -g -o server server.cpp \
    -lcapnp -lcapnp-rpc -lkj -lrdkafka -lrdkafka++ -lssl -lcrypto \
    -lcpprest -lboost_system -lboost_thread -lpthread

# Set the entry point
CMD ["/app/src/server"]
