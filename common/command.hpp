#pragma once

#include <iostream>
#include <memory>
#include <string>

namespace data {

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

  void operator()() {
    std::cout << "Task started" << std::endl;
    system(item_ptr_->cmd.data());
    std::cout.flush();
  }

private:
  std::unique_ptr<Command> item_ptr_;
};

} // namespace data
