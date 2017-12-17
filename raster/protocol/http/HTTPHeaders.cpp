/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPHeaders.h"
#include "raster/util/MapUtil.h"
#include "raster/util/String.h"

namespace rdd {

HTTPHeaders::HTTPHeaders(StringPiece sp) {
  StringPiece p = sp.split_step("\r\n");

  do {
    std::string line;
    while (!p.empty() && line.empty()) {
      toAppend(p, &line);
      p = sp.split_step("\r\n");
    }
    while (!p.empty() && isspace(p.front())) {
      toAppend(' ', ltrimWhitespace(p), &line);
      p = sp.split_step("\r\n");
    }
    StringPiece name, value;
    if (split(':', line, name, value)) {
      add(name.str(), trimWhitespace(value).str());
    }
  } while (!sp.empty());
}

bool HTTPHeaders::has(const std::string& name) const {
  return data_.find(normalizeName(name)) != data_.end();
}

void HTTPHeaders::set(const std::string& name, const std::string& value) {
  auto norm = normalizeName(name);
  data_.erase(norm);
  data_.emplace(norm, value);
}

void HTTPHeaders::add(const std::string& name, const std::string& value) {
  data_.emplace(normalizeName(name), value);
}

std::string HTTPHeaders::get(
    const std::string& name, const std::string& dflt) const {
  auto values = getList(name);
  return values.empty() ? dflt : join(",", values);
}

std::vector<std::string> HTTPHeaders::getList(const std::string& name) const {
  return get_all<std::vector<std::string>>(data_, normalizeName(name));
}

void HTTPHeaders::clearHeadersFor304() {
  // 304 responses should not contain entity headers (defined in
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec7.html#sec7.1)
  // not explicitly allowed by
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.3.5
  static std::vector<std::string> headers {
    "Allow", "Content-Encoding", "Content-Language", "Content-Length",
    "Content-Md5", "Content-Range", "Content-Type", "Last-Modified"
  };
  for (auto& h : headers) {
    data_.erase(h);
  }
}

std::string HTTPHeaders::normalizeName(const std::string& name) const {
  std::string norm(name);
  bool start = true;
  for (auto& c : norm) {
    c = start ? toupper(c) : tolower(c);
    start = c == '-';
  }
  return norm;
}

std::string HTTPHeaders::str() const {
  std::vector<std::string> v;
  for (auto& kv : data_) {
    v.emplace_back(std::move(to<std::string>(kv.first, ": ", kv.second)));
  }
  return join("\r\n", v);
}

} // namespace rdd