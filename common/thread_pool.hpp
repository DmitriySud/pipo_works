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

namespace thread_pool {

enum class OverflowPolicy {
  kAllow, kReject
};

template <typename Callable>
class ThreadPool {
public:
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

  bool AddTask(Callable&& task) {
    if (!waiters_.fetch_sub(1)) {
      waiters_.fetch_add(1);

      if (policy_ == OverflowPolicy::kReject) {
        return false;
      }
    }

    {
      std::unique_lock lock(wait_for_task_mt_);
      tasks_.push(std::move(task));
    }

    wait_for_task_cv_.notify_one();
    return true;
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

      Callable task;
      {
        std::unique_lock lock(wait_for_task_mt_);

        wait_for_task_cv_.wait(
            lock, [this] { return !tasks_.empty() || terminate_; });

        if (tasks_.empty()) {
          continue;
        }

        task = std::move(tasks_.front());
        tasks_.pop();
      }

      if (!terminate_) {
        task();
        wait_end_cv_.notify_one();
      }
    }
  }

  const size_t size_;
  OverflowPolicy policy_;

  std::vector<std::thread> threads_;
  std::queue<Callable> tasks_;

  std::atomic<int> waiters_;
  std::atomic<bool> terminate_{false};

  std::mutex wait_for_task_mt_;
  std::condition_variable wait_for_task_cv_;

  std::mutex wait_end_mt_;
  std::condition_variable wait_end_cv_;
};

} // namespace thread_pool
