

#include <capnp/message.h>
#include <capnp/schema-parser.h>
#include <capnp/serialize.h>
#include <cpprest/http_client.h>
#include <kj/io.h>

#include "HttpLog.hpp"

#pragma once

class CapncapDecoder {
 public:
  static HttpLog decodeMessagePayload(const char* decodeMessagePayload,
                                      size_t size) {
    kj::ArrayPtr<const capnp::word> array(
        reinterpret_cast<const capnp::word*>(decodeMessagePayload),
        size / sizeof(capnp::word));
    capnp::FlatArrayMessageReader reader(array);
    HttpLogRecord::Reader httpLogRecord = reader.getRoot<HttpLogRecord>();

    HttpLog httpLog(httpLogRecord);
    httpLog.anonymize();

    // std::cout << httpLog << std::endl;

    return httpLog;
  }
};