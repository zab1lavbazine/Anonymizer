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

#define CREATE_TABLE_HTTP_TRAFFIC_TOTALS_MV                        \
  "CREATE MATERIALIZED VIEW IF NOT EXISTS http_traffic_totals "    \
  "ENGINE = AggregatingMergeTree() "                               \
  "PARTITION BY toYYYYMM(timestamp) "                              \
  "ORDER BY (resource_id, http_status, cache_status, ip_address) " \
  "POPULATE "                                                      \
  "AS "                                                            \
  "SELECT "                                                        \
  "toStartOfDay(timestamp) AS timestamp, "                         \
  "resource_id, "                                                  \
  "response_status AS http_status, "                               \
  "cache_status, "                                                 \
  "remote_addr AS ip_address, "                                    \
  "SUM(bytes_sent) AS total_bytes_sent "                           \
  "FROM http_log "                                                 \
  "GROUP BY "                                                      \
  "timestamp, "                                                    \
  "resource_id, "                                                  \
  "response_status, "                                              \
  "cache_status, "                                                 \
  "remote_addr;"

#endif /* HTTP_LOG_QUERIES_H */
