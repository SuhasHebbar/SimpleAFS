#include <sys/types.h>
#include <vector>
#include <mutex>
#include <functional>
#include <thread>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <iostream>

// #define ALICE_CRASH

using FuncT = std::function<void()>;

using namespace std::chrono_literals;

class Delay {
public:
  void push(FuncT&& func) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(func);
  }

  void exec_inorder() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto&& func : queue_) {
      func();
    }
    queue_.clear();
  }

  void run() {
    while (true) {
      std::this_thread::sleep_for(131ms);
      exec_inorder();
    }
  }

  Delay() : thread_([this]() { run(); }) {}

private:
  std::vector<FuncT> queue_;
  std::mutex mutex_;
  std::thread thread_;
};

class Reorder {
public:
  void push(FuncT&& func) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(func);
  }

  void exec_reorder() {
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef ALICE_CRASH
    static volatile int num_writes = 0;
#endif

    std::random_shuffle(queue_.begin(), queue_.end());
    for (auto&& func : queue_) {
#ifdef ALICE_CRASH
      num_writes++;
      if (num_writes == 4) {
        std::cerr << "Crashing wiscAFS" << std::endl;
        exit(1);
      }
#endif
      func();
    }
    queue_.clear();
  }

  void run() {
    while (true) {
      std::this_thread::sleep_for(1930ms);
      exec_reorder();
    }
  }

  Reorder() : thread_([this]() { run(); }) {}

private:
  std::vector<FuncT> queue_;
  std::mutex mutex_;
  std::thread thread_;
};

namespace {
  std::unique_ptr<Delay> g_delay;
  std::unique_ptr<Reorder> g_reorder;
}

extern "C" {

void alice_init() {
  g_delay = std::make_unique<Delay>();
  g_reorder = std::make_unique<Reorder>();
}

void alice_destroy() {
  g_delay.reset();
  g_reorder.reset();
}

int alice_delay_pwrite(int fd, const char *buf, size_t size, off_t offset) {
  auto newbuf = new char[size];
  std::memcpy(newbuf, buf, size);
  g_delay->push([=]() {
    pwrite(fd, newbuf, size, offset);
    delete[] newbuf;
  });
  return size;
}

int alice_reorder_pwrite(int fd, const char *buf, size_t size, off_t offset) {
  auto newbuf = new char[size];
  std::memcpy(newbuf, buf, size);
  g_reorder->push([=]() {
    pwrite(fd, newbuf, size, offset);
    delete[] newbuf;
  });
  return size;
}

void alice_fsync(int fd) {
  // flush the whole buffer conservatively
  g_delay->exec_inorder();
  g_reorder->exec_reorder();
}

}
