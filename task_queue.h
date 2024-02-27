#ifndef _TASK_QUEUE_H_2024_
#define _TASK_QUEUE_H_2024_
/*
 *  Created by mobinsheng.
 */
#pragma once
#include <stdint.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

class QueuedTask {
 public:
  QueuedTask() {}
  virtual ~QueuedTask() {}
  virtual void run() = 0;

  // 重载函数调用运算符,直接调用run
  void operator()() { run(); }

 private:
  QueuedTask(const QueuedTask&) = delete;
};

template <class Closure>
class ClosureTask : public QueuedTask {
 public:
  explicit ClosureTask(Closure&& closure)
      : closure_(std::forward<Closure>(closure)) {}

  virtual void run() override { closure_(); }

 private:
  Closure closure_;
};

template <class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
  std::unique_ptr<QueuedTask> ptr(
      new ClosureTask<Closure>(std::forward<Closure>(closure)));
  return std::move(ptr);
}

typedef std::shared_ptr<QueuedTask> SharedClosure;

template <class Closure>
static SharedClosure MakeSharedClosure(Closure&& closure) {
  SharedClosure ptr(new ClosureTask<Closure>(std::forward<Closure>(closure)));
  return ptr;
}

/*
** 任务队列
*/
class TaskQueue {
 public:
  TaskQueue() : running_(false) {}

  void start() {
    if (running_) {
      return;
    }

    std::unique_lock<std::mutex> guard(mutex_);

    std::thread t(std::bind(&TaskQueue::run, this));
    thread_ = std::move(t);

    running_ = true;
  }

  void stop() {
    if (!running_) {
      return;
    }

    running_ = false;

    {
      std::unique_lock<std::mutex> guard(mutex_);

      task_list_.push_back(nullptr);

      cond_.notify_all();
    }

    thread_.join();
  }

  template <class Closure>
  bool add_task(Closure&& closure) {
    if (!running_) {
      return false;
    }
    std::unique_lock<std::mutex> guard(mutex_);
    std::unique_ptr<QueuedTask> task =
        NewClosure(std::forward<Closure>(closure));
    task_list_.push_back(std::move(task));
    cond_.notify_all();
    return true;
  }

 private:
  void run() {
    while (true) {
      std::unique_ptr<QueuedTask> task;
      {
        std::unique_lock<std::mutex> guard(mutex_);
        cond_.wait(guard, [&] { return !task_list_.empty(); });

        if (task_list_.empty()) {
          continue;
        }

        task = std::move(task_list_.front());
        task_list_.pop_front();
      }

      if (task == nullptr) {
        break;
      }

      task->run();
    }
  }

  std::mutex mutex_;
  std::condition_variable cond_;
  std::thread thread_;
  std::atomic<bool> running_;
  std::deque<std::unique_ptr<QueuedTask> > task_list_;
};

/*
void test(){
  TaskQueue q;
  q.start();

  q.add_task([&]{
    printf("test\n");
  });
}
*/

#endif // _TASK_QUEUE_H_2024_
