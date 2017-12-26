/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstdlib>
#include <utility>

namespace rdd {

class DeleterBase {
 public:
  virtual ~DeleterBase() {}
  virtual void dispose(void* pointer) const = 0;
};

template <class T>
class SimpleDeleter : public DeleterBase {
 public:
  void dispose(void* pointer) const override {
    delete static_cast<T*>(pointer);
  }
};

class DynamicPtr {
 public:
  DynamicPtr() : pointer_(nullptr), deleter_(nullptr) {}

  ~DynamicPtr() {
    dispose();
  }

  template <class T, class Deleter = SimpleDeleter<T>>
  void set(T* pointer) {
    if (pointer) {
      static auto deleter = new Deleter();
      pointer_ = pointer;
      deleter_ = deleter;
    }
  }

  template <class T>
  T* get() const {
    return static_cast<T*>(pointer_);
  }

  explicit operator bool() const {
    return pointer_ != nullptr;
  }

  void* release() {
    auto pointer = pointer_;
    pointer_ = nullptr;
    deleter_ = nullptr;
    return pointer;
  }

  bool dispose() {
    if (pointer_ != nullptr) {
      deleter_->dispose(pointer_);
      pointer_ = nullptr;
      deleter_ = nullptr;
      return true;
    }
    return false;
  }

  void swap(DynamicPtr& other) {
    std::swap(pointer_, other.pointer_);
    std::swap(deleter_, other.deleter_);
  }

  DynamicPtr(const DynamicPtr&) = delete;
  DynamicPtr& operator=(const DynamicPtr&) = delete;

 private:
  void* pointer_;
  DeleterBase* deleter_;
};

} // namespace rdd
