#ifndef HTTP_LOG_QUERIES_H
#define HTTP_LOG_QUERIES_H

#define CLICKHOUSE_HOST "http://localhost:8123"

#define INSERT_INTO_DATABASE_HTTP_LOG                           \
  "INSERT INTO http_log (timestamp, resource_id, bytes_sent, "  \
  "request_time_milli, response_status, cache_status, method, " \
  "remote_addr, url) VALUES"

#define CREATE_TABLE_HTTP_LOG                 \
  "CREATE TABLE IF NOT EXISTS http_log ("     \
  "    timestamp DateTime, "                  \
  "    resource_id UInt64, "                  \
  "    bytes_sent UInt64, "                   \
  "    request_time_milli UInt64, "           \
  "    response_status UInt16, "              \
  "    cache_status LowCardinality(String), " \
  "    method LowCardinality(String), "       \
  "    remote_addr String, "                  \
  "    url String "                           \
  ") ENGINE = MergeTree() "                   \
  "ORDER BY (timestamp) "                     \
  "SETTINGS index_granularity = 8192;"

#define CREATE_TABLE_HTTP_TRAFFIC_TOTALS_MV                           \
  "CREATE MATERIALIZED VIEW IF NOT EXISTS traffic_totals_mv "         \
  "ENGINE = MergeTree() "                                             \
  "PARTITION BY toYYYYMM(timestamp) "                                 \
  "ORDER BY (timestamp, resource_id, response_status, cache_status, " \
  "remote_addr) "                                                     \
  "AS "                                                               \
  "SELECT "                                                           \
  "    timestamp, "                                                   \
  "    resource_id, "                                                 \
  "    response_status, "                                             \
  "    cache_status, "                                                \
  "    remote_addr, "                                                 \
  "    sum(bytes_sent) AS total_bytes_sent, "                         \
  "    sum(request_time_milli) AS total_request_time "                \
  "FROM "                                                             \
  "    http_log "                                                     \
  "GROUP BY "                                                         \
  "    timestamp, "                                                   \
  "    resource_id, "                                                 \
  "    response_status, "                                             \
  "    cache_status, "                                                \
  "    remote_addr;"

#endif /* HTTP_LOG_QUERIES_H */
