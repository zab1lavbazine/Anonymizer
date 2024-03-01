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
    cmake \
    make \
    capnproto \
    iproute2 \
    net-tools \
    iputils-ping \
    curl

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY src/ /app/src/
COPY Makefile /app/

# Compile the project using the Makefile
RUN cd /app && \
    make

# Expose the necessary ports
EXPOSE 9092 8124

# Set the entry point
CMD ["/app/src/server"]
