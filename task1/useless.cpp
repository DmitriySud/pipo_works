#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <queue>
#include <string>

#include <stdlib.h>
#include <unistd.h>

#include <common/thread_pool.hpp>
#include <common/command.hpp>

namespace chrono = std::chrono;
using TimePoint = chrono::system_clock::time_point;

namespace data {

struct Item final : public Command {
  size_t delay;

  struct ItemMore {
    bool operator()(const Item &lhs, const Item &rhs) {
      return lhs.delay > rhs.delay;
    }
  };
};

using Container = std::vector<Item>;

} // namespace data

namespace {

static constexpr size_t kThreadPoolSize{10};

std::string ExtractFilename(int argc, char *argv[]) {
  if (argc != 2) {
    throw std::runtime_error("wrong args amount");
  }

  return std::string(argv[1]);
}

std::optional<data::Item> ExtractRecord(std::istream &stream) {
  data::Item res;

  if (stream >> res.delay) {
    std::getline(stream, res.cmd);
    return res;
  }

  return std::nullopt;
}

} // namespace

using Queue =
    std::priority_queue<data::Item, data::Container, data::Item::ItemMore>;

int main(int argc, char *argv[]) {
  TimePoint start_time = chrono::system_clock::now();

  std::ifstream fin(ExtractFilename(argc, argv));

  thread_pool::ThreadPool<data::CommandLauncher> tp{
      kThreadPoolSize, thread_pool::OverflowPolicy::kAllow};
  Queue records;

  while (auto it = ExtractRecord(fin)) {
    records.emplace(*it);
  }

  std::cout << "Strat process records" << std::endl;

  while (!records.empty()) {
    std::unique_ptr<data::Item> it =
        std::make_unique<data::Item>(std::move(records.top()));

    TimePoint expected_time = start_time + chrono::seconds(it->delay);

    TimePoint now_time = chrono::system_clock::now();

    if (now_time <= expected_time) {
      auto delta = expected_time - now_time;
      usleep(chrono::duration_cast<chrono::microseconds>(delta).count());
    }

    auto time = data::GetTime();
    tp.AddTask(data::CommandLauncher(std::move(it)));

    records.pop();
  }

  return 0;
}
