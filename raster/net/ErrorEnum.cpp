/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/net/ErrorEnum.h"

#define RDD_NET_ERROR_STR(error) #error

namespace {
  static const char* netErrorStrings[] = {
    RDD_NET_ERROR_GEN(RDD_NET_ERROR_STR)
  };
}

namespace rdd {

const char* getNetErrorString(NetError error) {
  if (error < kErrorNone || error >= kErrorMax) {
    return netErrorStrings[kErrorMax];
  } else {
    return netErrorStrings[error];
  }
}

const char* getNetErrorStringByIndex(int i) {
  return netErrorStrings[i];
}

} // namespace rdd
