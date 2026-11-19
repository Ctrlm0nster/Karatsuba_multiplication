#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace km {

/**
 * @brief Thread pool for parallelizing recursive divide-and-conquer sub-problems.
 *
 * Implements a task queue with worker threads, safe for concurrent task submission
 * and verified under ThreadSanitizer.
 */
class ThreadPool {
public:
  /**
   * @brief Access the shared global thread pool instance.
   * @return Reference to the singleton ThreadPool.
   */
  static ThreadPool& instance() {
    static ThreadPool pool(std::max(2u, std::thread::hardware_concurrency()));
    return pool;
  }

  /**
   * @brief Construct a ThreadPool with a specific number of worker threads.
   * @param numThreads Number of worker threads to spawn.
   */
  explicit ThreadPool(std::size_t numThreads = std::max(2u, std::thread::hardware_concurrency()))
      : stop_(false) {
    for (std::size_t i = 0; i < numThreads; ++i) {
      workers_.emplace_back([this]() {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
              return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
          }
          task();
        }
      });
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (std::thread& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  /**
   * @brief Enqueue a callable task for asynchronous execution.
   * @param f Function or lambda to execute.
   * @param args Arguments to pass to f.
   * @return std::future holding the return value of f.
   */
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      if (stop_) {
        throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
      }
      tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return res;
  }

  /**
   * @brief Return the number of worker threads in the pool.
   */
  [[nodiscard]] std::size_t threadCount() const noexcept {
    return workers_.size();
  }

private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queueMutex_;
  std::condition_variable cv_;
  bool stop_{false};
};

}  // namespace km
