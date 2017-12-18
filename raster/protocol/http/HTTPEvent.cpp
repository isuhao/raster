/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/net/Protocol.h"
#include "raster/protocol/http/HTTPEvent.h"
#include "raster/protocol/http/Util.h"

namespace rdd {

HTTPEvent::HTTPEvent(const std::shared_ptr<Channel>& channel,
                     const std::shared_ptr<Socket>& socket)
  : Event(channel, socket) {
  state_ = INIT;
}

void HTTPEvent::reset() {
  Event::reset();
  state_ = ON_READING_HEADERS;
}

void HTTPEvent::onReadingHeaders() {
  IOBuf* buf = rbuf().get();
  headerSize_ = buf->computeChainDataLength();

  StringPiece data(buf->coalesce());
  auto eol = data.find("\r\n");
  StringPiece method, uri, version;

  if (!split(' ', data.subpiece(0, eol), method, uri, version)) {
    RDDLOG(INFO) << *this << " malformed HTTP request line";
    state_ = ERROR;
    return;
  }
  if (!version.startsWith("HTTP/")) {
    RDDLOG(INFO) << *this << " malformed HTTP version in HTTP request";
    state_ = ERROR;
    return;
  }

  HTTPHeaders headers;
  headers.parse(data.subpiece(eol + 2));

  request_ = std::make_shared<HTTPRequest>(
      method, uri, version, std::move(headers), true, peer().host);

  auto clen = request_->headers.getSingleOrEmpty(HTTP_HEADER_CONTENT_LENGTH);

  if (clen.empty()) {
    state_ = ON_READING_FINISH;
  } else {
    size_t n = to<size_t>(clen);
    if (n > Protocol::BODYLEN_LIMIT) {
      RDDLOG(WARN) << *this << " big request, bodyLength=" << n;
    } else {
      RDDLOG(V3) << *this << " bodyLength=" << n;
    }
    /* TODO
    if (request_->headers.get("Expect") == "100-continue") {
      wbuf()->append("HTTP/1.1 100 (Continue)\r\n\r\n");
    }*/
    rlen() += n;
    state_ = ON_READING_BODY;
  }
}

void HTTPEvent::onReadingBody() {
  IOBuf* buf = rbuf().get();
  StringPiece data(buf->coalesce());
  request_->body = data.subpiece(headerSize_);

  if (request_->method == "POST" ||
      request_->method == "PATCH" ||
      request_->method == "PUT") {
    parseBodyArguments(
        request_->headers.getSingleOrEmpty(HTTP_HEADER_CONTENT_TYPE),
        data, request_->arguments, request_->files);
  }
  state_ = ON_READING_FINISH;
}

void HTTPEvent::onWritingFinish() {
  if (response_->statusCode == 200 &&
      (request_->method == "GET" || request_->method == "HEAD") &&
      !response_->headers.exists(HTTP_HEADER_ETAG)) {
    auto etag = response_->computeEtag();
    if (!etag.empty()) {
      response_->headers.set(HTTP_HEADER_ETAG, etag);
      auto inm = request_->headers.combine(HTTP_HEADER_IF_NONE_MATCH);
      if (!inm.empty() && inm.find(etag) != std::string::npos) {
        response_->data->clear();
        response_->statusCode = 304;
      }
    }
  }
  if (response_->statusCode == 304) {
    response_->headers.clearHeadersFor304();
  } else if (response_->headers.exists(HTTP_HEADER_CONTENT_LENGTH)) {
    auto clen = response_->data->computeChainDataLength();
    response_->headers.set("Content-Length", to<std::string>(clen));
  }
  if (request_->method == "HEAD") {
    response_->data->clear();
  }
  response_->prependHeaders(request_->version);
  if (response_->statusCode < 400) {
    RDDLOG(INFO) << *this << response_->statusCode;
  } else if (response_->statusCode < 500) {
    RDDLOG(WARN) << *this << response_->statusCode;
  } else {
    RDDLOG(ERROR) << *this << response_->statusCode;
  }
  wbuf().swap(response_->data);
  state_ = ON_WRITING_FINISH;
}

} // namespace rdd
