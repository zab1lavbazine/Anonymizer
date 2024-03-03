# Anonymizer


This project is a logging pipeline designed to consume HTTP records from an Apache Kafka topic (`http_log`), decode them using Cap'N Proto, and insert them into a ClickHouse table via HTTP requests.


## Features

- **Real-time Processing**: Consumes HTTP log messages in real-time from a Kafka topic.
- **Cap'N Proto Decoding**: Decodes the payload of Kafka messages using Cap'N Proto schema.
- **Anonymization**: Anonymizes client IP addresses to comply with GDPR regulations.
- **Reliable Insertion**: Inserts HTTP log data into ClickHouse tables reliably, ensuring data integrity.
- **Error Handling**: Handles errors gracefully and retries failed operations.
- **Rate Limiting**: Communicates with ClickHouse server through a proxy with rate limiting.
- **Scalability**: Can scale to handle larger volumes of HTTP log data efficiently.


## Components

### KafkaHandler

Coordinates the Kafka consumer and HTTP sender components. It configures and manages the Kafka consumer to consume messages from the Kafka topic and the HTTP sender to send HTTP requests to ClickHouse.


### KafkaConsumer 

A Kafka consumer class responsible for consuming messages from the Kafka topic. It uses the librdkafka library to interface with Kafka. When a message is received, it sends the message to the CapncapDecoder that returns the decoded message. After that, it sends this message to the ThreadSafeQueue.

### HttpSender   

A class responsible for the sending of HTTP requests to ClickHouse. It uses the libcurl library to interface with ClickHouse. It sends the HTTP request to the ClickHouse server via ch-proxy and returns the response.


### Supporting Classes

- **CapnCapDecoder**: A class responsible for decoding the Cap'N Proto encoded messages.
- **ThreadSafeQueue**: A thread-safe queue that allows multiple threads to safely enqueue and dequeue messages.
- **OutputHandler**: A class responsible for handling the output of the program. It logs the output to the console and to a file.
- **HttpLog**: A class that represents the HTTP log message. It contains the fields of the HTTP log message and methods to serialize and deserialize the message.

### Other Components

- **HttpLogQueries**: A class that contains the queries to create the ClickHouse table and insert data into the table.




## Possible problems

### Latency of the Data:

- **Limitations**: The latency of the data can vary depending on factors such as network latency, processing time for decoding Cap'N Proto messages.
- **Unbounded Message Queue**: The Kafka consumer pushes messages into an in-memory queue (innerHttpLogQueue) without any bounds or limits on its size. This could lead to excessive memory consumption, especially during periods of high message influx, and may result in out-of-memory errors or system instability.
- **High Message Consumption Rate**: The Kafka consumer is consuming messages from the Kafka topic at a constant rate, regardless of the rate at which the messages are being produced.
- **Data Persistence**: Consider implementing mechanisms for data persistence or backup to prevent data loss in case of system failures or crashes. This could involve periodically flushing the message queue to persistent storage or leveraging Kafka features such as message offsets for message replayability.
- **Lack of Duplicate Record Mechanism**: There is no mechanism in place to prevent the storage of duplicate records in ClickHouse.