/*
*  Created by mobinsheng.
*/
#ifndef _LAZY_TASK_QUEUE_H_
#define _LAZY_TASK_QUEUE_H_
#pragma once
#include <assert.h>
#include <stdint.h>
#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>
#include <memory>
#include <type_traits>
#include <utility>
#include <functional>
#include <atomic>
#include <map>

namespace lazy {

class QueuedTask {
public:
	QueuedTask() {}
	virtual ~QueuedTask() {}
	virtual void run() = 0;

	// 重载函数调用运算符,直接调用run
	void operator()() { run(); }

	// is sync task
	bool is_sync = false;

	// is task finish
	bool finished = false;

	int64_t invoke_ms = 0;
private:
	QueuedTask(const QueuedTask&) = delete;
};

template <class ReturnT, class Closure>
class ClosureTask : public QueuedTask {
public:
	explicit ClosureTask(Closure&& closure) : closure_(std::forward<Closure>(closure)) {}

	virtual void run() override {
		result_ = closure_();
	}

	ReturnT move_result() { return std::move(result_); }

private:
	Closure closure_;
	ReturnT result_;
};

template <class Closure>
class ClosureTask<void, Closure> : public QueuedTask {
public:
	explicit ClosureTask(Closure&& closure) : closure_(std::forward<Closure>(closure)) {}

	virtual void run() override {
		closure_();
	}

	void move_result() { }

private:
	Closure closure_;
};


template <class ReturnT, class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
	std::unique_ptr<QueuedTask> ptr(new ClosureTask<ReturnT, Closure>(std::forward<Closure>(closure)));
	return std::move(ptr);
}

template <class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
	std::unique_ptr<QueuedTask> ptr(new ClosureTask<void, Closure>(std::forward<Closure>(closure)));
	return std::move(ptr);
}

typedef std::shared_ptr<QueuedTask> SharedClosure;

template <class ReturnT, class Closure>
static SharedClosure MakeSharedClosure(Closure&& closure) {
	SharedClosure ptr(new ClosureTask<ReturnT, Closure>(std::forward<Closure>(closure)));
	return ptr;
}

template <class Closure>
static SharedClosure MakeSharedClosure(Closure&& closure) {
	SharedClosure ptr(new ClosureTask<void, Closure>(std::forward<Closure>(closure)));
	return ptr;
}


// Time Utils
class TimeUtil {
public:
	// milliseconds
	static void SleepMs(uint64_t ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

	// microseconds
	static void SleepUs(uint64_t us) {
		std::this_thread::sleep_for(std::chrono::microseconds(us));
	}

	// nanoseconds
	static void SleepNs(uint64_t ns) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
	}

	// milliseconds
	static int64_t NowMs() {
		auto duration = std::chrono::system_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	}

	// microseconds
	static int64_t NowUs() {
		auto duration = std::chrono::system_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
	}

	// nanoseconds
	static int64_t NowNs() {
		auto duration = std::chrono::system_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	}
};

/*
** 任务队列
*/
class TaskQueue {
public:
	TaskQueue() {}

	~TaskQueue() {
		if (thread_.joinable()) {
			{
				std::unique_lock<std::mutex> guard(mutex_);

				flush_ = true;

				if (thread_.joinable()) {

					//task_list_.push_back(nullptr);
					delayed_task_map_.insert(std::make_pair(TimeUtil::NowMs(), nullptr));

					cond_.notify_one();
				}
			}
			thread_.join();
		}

	}

	void start() {
		maybe_create_thread();
	}

	void stop() {

	}

	bool is_current() {
		return std::this_thread::get_id() == thread_.get_id();
	}

	void set_name(const std::string& name) {
		std::unique_lock<std::mutex> guard(mutex_);
		name_ = name;
	}

	const std::string& name() const {
		return name_;
	}

	template <class Closure>
	void add_task(Closure&& closure) {
		post_delayed(std::forward<Closure>(closure), 0);
	}

	template <class Closure>
	void post(Closure&& closure) {
		post_delayed(std::forward<Closure>(closure), 0);
	}

	template <class Closure>
	void post_delayed(Closure&& closure, uint32_t delay_ms) {
		maybe_create_thread();

		std::unique_lock<std::mutex> guard(mutex_);
		std::shared_ptr<QueuedTask> task = MakeSharedClosure<void, Closure>(std::forward<Closure>(closure));
		if (delay_ms > 0) {
			task->invoke_ms = TimeUtil::NowMs() + delay_ms;
		}
		delayed_task_map_.insert(std::make_pair(task->invoke_ms, std::move(task)));
		cond_.notify_one();
	}


	template <class ReturnT, class Closure>
	ReturnT invoke(Closure&& closure) {
		maybe_create_thread();

		std::shared_ptr<QueuedTask> task = MakeSharedClosure<ReturnT, Closure>(std::forward<Closure>(closure));
		task->is_sync = true;
		{
			std::unique_lock<std::mutex> guard(mutex_);
			task_list_.push_back(task);
			cond_.notify_one();
		}
		{
			std::unique_lock<std::mutex> guard(sync_mutex_);
			sync_cond_.wait(guard, [&] {
				return task->finished;
			});
		}
		ClosureTask<ReturnT, Closure>* ptr = (ClosureTask<ReturnT, Closure>*)task.get();
		return ptr->move_result();
	}
private:

	void maybe_create_thread() {

		if (thread_.joinable()) {
			return;
		}

		std::unique_lock<std::mutex> guard(mutex_);

		if (thread_.joinable()) {
			return;
		}

		thread_ = std::thread(std::bind(&TaskQueue::run, this));
	}

	void run() {
		while (true) {
			std::shared_ptr<QueuedTask> task;

			bool sleep = false;

			bool exit_loop = false;

			do
			{
				std::unique_lock<std::mutex> guard(mutex_);
				cond_.wait(guard, [&] {
					return (!task_list_.empty()) || (!delayed_task_map_.empty());
				});

				if (task_list_.empty() && delayed_task_map_.empty()) {
					continue;
				}

				int64_t now_ms = TimeUtil::NowMs();

				for (auto it = delayed_task_map_.begin(); it != delayed_task_map_.end();) {
					if (it->first <= now_ms || flush_) {
						task_list_.push_back(std::move(it->second));
						it = delayed_task_map_.erase(it);
					}
					else {
						break;
					}
				}

				if (task_list_.empty()) {
					if (delayed_task_map_.empty()) {
						// just wait
						continue;
					}
					else {
						// wait delayed task timeout
						sleep = true;

					}
				}
				else {
					task = std::move(task_list_.front());
					task_list_.pop_front();

					if (task == nullptr) {
						exit_loop = true;
					}
				}

			} while (0);

			if (exit_loop) {
				break;
			}

			if (sleep) {
				//std::this_thread::yield();s
				TimeUtil::SleepUs(500);
				continue;
			}

			assert(task != nullptr);

			task->run();

			if (task->is_sync) {
				std::unique_lock<std::mutex> guard(sync_mutex_);
				task->finished = true;
				sync_cond_.notify_all();
			}
		}
	}

	std::mutex mutex_;
	std::condition_variable cond_;

	std::thread thread_;

	std::deque<std::shared_ptr<QueuedTask>> task_list_;

	std::mutex sync_mutex_;
	std::condition_variable sync_cond_;

	std::multimap<uint64_t/*time ms*/, std::shared_ptr<QueuedTask>> delayed_task_map_;

	bool flush_ = false;

	std::string name_ = "";
};

}

#endif // _LAZY_TASK_QUEUE_H_
