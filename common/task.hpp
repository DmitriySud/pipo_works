#pragma once

#include <atomic>
#include <memory>

#include <iostream>

namespace thread_pool {

template <typename Callable> 
class Task {
public:
  Task(Callable &&run) : run_(std::move(run)) {}

  void Execute() {
    run_();
    done_.store(true);
    done_.notify_one();
  }

  void Wait() { done_.wait(false); }

private:
  Callable run_;
  std::atomic<int> done_{false};
};

template <typename Callable> 
class TaskWrapper {
public:
  TaskWrapper(Callable &&run)
      : ptr_(std::make_shared<Task<Callable>>(std::move(run))) {}
  TaskWrapper() : ptr_(nullptr) {}

  std::shared_ptr<Task<Callable>> GetPtr() const { return ptr_; }

private:
  std::shared_ptr<Task<Callable>> ptr_{nullptr};
};
} // namespace thread_pool
