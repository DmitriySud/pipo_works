#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <ctime>
#include <unistd.h>

namespace data {

static constexpr size_t kMaxArgs = 10;

static time_t GetTime() {
  return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

struct Command {
  Command(const std::string &cmd) : cmd(cmd){};
  Command(const Command &other) = default;
  Command() = default;
  std::string cmd{};
};

class CommandLauncher {
public:
  CommandLauncher(std::unique_ptr<Command> &&ptr) : item_ptr_(std::move(ptr)) {}
  CommandLauncher(CommandLauncher&&) = default;
  CommandLauncher() = default;
  CommandLauncher &operator=(CommandLauncher &&) = default;

  void operator()() {
    size_t task_num = counter++;
    auto time = GetTime();
    std::cout << "Task " << task_num << " started at " << ctime(&time)
              << std::endl;

    int status = -1;
    std::cout << execCommand(item_ptr_->cmd, status);

    time = GetTime();
    std::cout << "Task " << task_num << " finished with status " << status
              << "at " << ctime(&time) << std::endl;
    std::cout.flush();
  }

private:
  std::string execCommand(const std::string cmd, int &out_exitStatus) {
    out_exitStatus = 0;
    auto pPipe = ::popen(cmd.c_str(), "r");
    if (pPipe == nullptr) {
      throw std::runtime_error("Cannot open pipe");
    }

    std::array<char, 256> buffer;

    std::string result;

    while (not std::feof(pPipe)) {
      auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
      result.append(buffer.data(), bytes);
    }

    auto rc = ::pclose(pPipe);

    if (WIFEXITED(rc)) {
      out_exitStatus = WEXITSTATUS(rc);
    }

    return result;
  }

  inline static std::atomic<size_t> counter{0};
  std::unique_ptr<Command> item_ptr_;
};

} // namespace data
