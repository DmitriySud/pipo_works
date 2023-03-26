#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <iostream>
#include <cassert>

#include "task.hpp"

namespace thread_pool {

enum class OverflowPolicy {
  kAllow, kReject
};

template <typename Callable>
class ThreadPool {
public:
  using TaskPtr =std::shared_ptr<Task<Callable>>; 

  ThreadPool(size_t size, OverflowPolicy policy)
      : size_(size), policy_(policy) {

    assert(size > 0);

    waiters_.store(0);
    threads_.reserve(size_);
    for (int i = 0; i < size_; i++) {
      threads_.emplace_back(std::bind(&ThreadPool::ThreadTask, this));
    }
    while (!Started())
      ;
  }

  void WaitForTasks() {
    std::unique_lock lock(wait_end_mt_);

    wait_end_cv_.wait(lock, [this]() { return tasks_.empty(); });
  }

  TaskPtr AddTask(Callable&& task) {
    if (!waiters_.fetch_sub(1)) {
      waiters_.fetch_add(1);

      if (policy_ == OverflowPolicy::kReject) {
        return nullptr;
      }
    }

    TaskPtr res {nullptr};
    {
      std::unique_lock lock(wait_for_task_mt_);
      TaskWrapper<Callable> wrapper(std::move(task));
      res = wrapper.GetPtr();
      tasks_.push(std::move(wrapper));
    }

    wait_for_task_cv_.notify_one();
    return res;
  }

  bool Started() const { return waiters_.load() == size_; }

  ~ThreadPool() {
    WaitForTasks();
    Terminate();
  }

private:
  void Terminate() {
    {
      std::unique_lock lock(wait_for_task_mt_);
      terminate_.store(true);
    }

    wait_for_task_cv_.notify_all();

    for (std::thread& thread : threads_) {
      thread.join();
    }
  }

  void ThreadTask() {
    while (!terminate_) {
      waiters_.fetch_add(1);

      TaskWrapper<Callable> wrapper;
      {
        std::unique_lock lock(wait_for_task_mt_);

        wait_for_task_cv_.wait(
            lock, [this] { return !tasks_.empty() || terminate_; });

        if (tasks_.empty()) {
          continue;
        }

        wrapper = std::move(tasks_.front());
        tasks_.pop();
      }

      if (!terminate_) {
        wrapper.GetPtr()->Execute();
        wait_end_cv_.notify_one();
      }
    }
  }

  const size_t size_;
  OverflowPolicy policy_;

  std::vector<std::thread> threads_;

  // Use same approach as std::unordered_map. We have to use wrapper because
  // std::queue has paged-vector as underlying type and pointers to it's items
  // can be invalidated.
  std::queue<TaskWrapper<Callable>> tasks_;

  std::atomic<int> waiters_;
  std::atomic<bool> terminate_{false};

  std::mutex wait_for_task_mt_;
  std::condition_variable wait_for_task_cv_;

  std::mutex wait_end_mt_;
  std::condition_variable wait_end_cv_;
};

} // namespace thread_pool
