# Use an appropriate base image
FROM ubuntu:latest

# Install required libraries and tools
RUN apt-get update && apt-get install -y \
    g++ \
    libcapnp-dev \
    cmake \
    make \
    capnproto \
    librdkafka-dev \
    libssl-dev \
    libcpprest-dev \
    libboost-system-dev \
    libboost-thread-dev

# Set the working directory
WORKDIR /Anonymizer

# Copy the source code and Makefile into the container
COPY src/ ./src
COPY Makefile ./

RUN mkdir compiled
RUN mkdir output

# Compile the project
RUN make all



# Run the application
CMD ["./compiled/anonymizer"]
