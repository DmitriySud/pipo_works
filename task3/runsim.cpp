#include <iostream>
#include <string>

#include <unistd.h>
#include <stdlib.h>

#include <common/thread_pool.hpp>
#include <common/command.hpp>

int main(int argc, char* argv[]) {
  size_t threads = 0;

  try {
    threads = std::stoul(argv[1]);
  } catch (const std::exception &ex) {
    std::cerr << "Cannot parse number of threads " << ex.what() << std::endl;
    return 0;
  }

  thread_pool::ThreadPool<data::CommandLauncher> tp{
      threads, thread_pool::OverflowPolicy::kReject};

  data::Command command{};
  while (std::getline(std::cin, command.cmd)) {
    if (!tp.AddTask(
            data::CommandLauncher(std::make_unique<data::Command>(command)))) {
      std::cout << "Cannot start more than " << threads << " tasks"
                << std::endl;
    }
  }

  return 0;
}
