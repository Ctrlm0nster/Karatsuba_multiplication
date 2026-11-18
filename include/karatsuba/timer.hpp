#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>

namespace km {

class Timer {
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  Timer();
  ~Timer() = default;

  void start();
  void stop();
  void reset();
  void restart();

  [[nodiscard]] bool isRunning() const;
  [[nodiscard]] double elapsedSeconds() const;
  [[nodiscard]] double elapsedMilliseconds() const;
  [[nodiscard]] double elapsedMicroseconds() const;
  [[nodiscard]] std::int64_t elapsedNanoseconds() const;

  // Backwards compatibility methods
  void Start() {
    start();
  }
  void Stop() {
    stop();
  }
  void Reset() {
    reset();
  }
  [[nodiscard]] double GetSeconds() const {
    return elapsedSeconds();
  }
  [[nodiscard]] double GetMilliseconds() const {
    return elapsedMilliseconds();
  }
  void DisplayTime() const;

private:
  TimePoint m_startTime{};
  TimePoint m_endTime{};
  bool m_isRunning{false};
  mutable std::mutex m_mutex{};
};

class ScopedTimer {
public:
  using Callback = std::function<void(double elapsedMs)>;

  explicit ScopedTimer(std::string name);
  explicit ScopedTimer(Callback callback);
  ~ScopedTimer();

  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
  std::string m_name;
  Callback m_callback;
  Timer m_timer;
};

}  // namespace km

// Global alias for backwards compatibility
using Timer = km::Timer;
