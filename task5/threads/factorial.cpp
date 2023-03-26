#include <common/big_integer.hpp>
#include <common/thread_pool.hpp>

#include <iostream>

namespace {
  using Integer = big_numbers::BigInteger;
  using ThreadPool = thread_pool::ThreadPool<std::function<void()>>;
}

int main(int argc, char** argv) {
  size_t threads = 0;

  if (argc < 2) {
    threads = std::thread::hardware_concurrency() + 1;
  } else {
    try {
      threads = std::stoul(argv[1]);
    } catch (const std::exception &ex) {
      std::cerr << "Cannot parse number of threads " << ex.what() << std::endl;
      threads = std::thread::hardware_concurrency() + 1;
    }
  }

  ThreadPool tp{threads, thread_pool::OverflowPolicy::kAllow};

  int next;
  while (std::cin >> next) {
    if (next < 0){
      std::cerr << "Cannot calculate negative factorial" << std::endl;
      continue;
    }

    tp.AddTask([next] {
      Integer res{1};
      for (uint32_t cur = 2; cur <= next; ++cur) {
        res *= cur;
      }

      std::cout << next << "! = " << res << std::endl;
    });
  }

  return 0;
}
