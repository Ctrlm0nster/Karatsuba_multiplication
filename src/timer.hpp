#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>

class Timer {
public:
  Timer();
  ~Timer() = default;
  void Start();
  void Stop();
  void Reset();
  double GetSeconds() const;
  double GetMilliseconds() const;
  void DisplayTime() const;

private:
  std::chrono::high_resolution_clock::time_point m_startTime;
  std::chrono::high_resolution_clock::time_point m_endTime;
  std::atomic<bool> m_IsRunning;
  mutable std::mutex m_Mutex;
};
