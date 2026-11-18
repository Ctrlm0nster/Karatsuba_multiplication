#include "karatsuba/timer.hpp"

#include <iomanip>
#include <iostream>

namespace km {

Timer::Timer() {
  reset();
}

void Timer::start() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isRunning) {
    m_startTime = Clock::now();
    m_isRunning = true;
  }
}

void Timer::stop() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_isRunning) {
    m_endTime = Clock::now();
    m_isRunning = false;
  }
}

void Timer::reset() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_startTime = Clock::now();
  m_endTime = m_startTime;
  m_isRunning = false;
}

void Timer::restart() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_startTime = Clock::now();
  m_endTime = m_startTime;
  m_isRunning = true;
}

bool Timer::isRunning() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_isRunning;
}

double Timer::elapsedSeconds() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto end = m_isRunning ? Clock::now() : m_endTime;
  return std::chrono::duration<double>(end - m_startTime).count();
}

double Timer::elapsedMilliseconds() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto end = m_isRunning ? Clock::now() : m_endTime;
  return std::chrono::duration<double, std::milli>(end - m_startTime).count();
}

double Timer::elapsedMicroseconds() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto end = m_isRunning ? Clock::now() : m_endTime;
  return std::chrono::duration<double, std::micro>(end - m_startTime).count();
}

std::int64_t Timer::elapsedNanoseconds() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto end = m_isRunning ? Clock::now() : m_endTime;
  return std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_startTime).count();
}

void Timer::DisplayTime() const {
  const double seconds = elapsedSeconds();
  const double milliseconds = elapsedMilliseconds();
  std::cout << std::fixed << std::setprecision(6) << "Elapsed Time: " << seconds << " seconds ("
            << milliseconds << " ms)\n";
}

ScopedTimer::ScopedTimer(std::string name) : m_name(std::move(name)), m_callback(nullptr) {
  m_timer.start();
}

ScopedTimer::ScopedTimer(Callback callback) : m_name(""), m_callback(std::move(callback)) {
  m_timer.start();
}

ScopedTimer::~ScopedTimer() {
  m_timer.stop();
  const double elapsed = m_timer.elapsedMilliseconds();
  if (m_callback) {
    m_callback(elapsed);
  } else if (!m_name.empty()) {
    std::cout << "[Timer] " << m_name << ": " << std::fixed << std::setprecision(3) << elapsed
              << " ms\n";
  }
}

}  // namespace km
