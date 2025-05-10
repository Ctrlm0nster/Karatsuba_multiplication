#include "timer.hpp"
#include <chrono>
#include <mutex>
#include <ratio>

// Default Constructor
Timer::Timer() : m_IsRunning(false) { Reset(); }

// Starts the timer
void Timer::Start() {
  bool expected = false;
  if (m_IsRunning.compare_exchange_strong(expected, true)) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_startTime = std::chrono::high_resolution_clock::now();
  }
}

// Stop the timer
void Timer::Stop() {
  bool expected = true;
  if (m_IsRunning.compare_exchange_strong(expected, false)) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_endTime = std::chrono::high_resolution_clock::now();
  }
}

// Reset the clock
void Timer::Reset() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_startTime = std::chrono::high_resolution_clock::now();
  m_endTime = m_startTime;
  m_IsRunning.store(false);
}

double Timer::GetSeconds() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::chrono::time_point<std::chrono::high_resolution_clock> endTime;

  if (m_IsRunning.load()) {
    endTime = std::chrono::high_resolution_clock::now();
  }
  else {
    endTime = m_endTime;
  }
  return std::chrono::duration<double>(endTime - m_startTime).count();
}

double Timer::GetMilliseconds() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::chrono::time_point<std::chrono::high_resolution_clock> endTime;

  if (m_IsRunning.load()) {
    endTime = std::chrono::high_resolution_clock::now();
  }
  else {
    endTime = m_endTime;
  }
  return std::chrono::duration<double, std::milli>(endTime - m_startTime)
    .count();
}

void Timer::DisplayTime() const {
  double seconds = GetSeconds();
  double milliseconds = GetMilliseconds();

  std::cout << "Elapsed Time: " << seconds << " seconds" << '\n';
  std::cout << "Elapsed Time: " << milliseconds << " milliseconds" << '\n';
}
