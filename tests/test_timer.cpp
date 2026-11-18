#include <chrono>
#include <thread>

#include "karatsuba/timer.hpp"
#include "test_framework.hpp"

using namespace km;

TEST_CASE(Timer, BasicMeasurement) {
  Timer timer;
  ASSERT_FALSE(timer.isRunning());
  ASSERT_EQ(timer.elapsedNanoseconds(), 0);

  timer.start();
  ASSERT_TRUE(timer.isRunning());
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  timer.stop();
  ASSERT_FALSE(timer.isRunning());

  ASSERT_TRUE(timer.elapsedMilliseconds() >= 4.0);
  ASSERT_TRUE(timer.elapsedSeconds() >= 0.004);

  // Restart
  timer.restart();
  ASSERT_TRUE(timer.isRunning());
  timer.stop();

  // Reset
  timer.reset();
  ASSERT_FALSE(timer.isRunning());
  ASSERT_EQ(timer.elapsedNanoseconds(), 0);
}

TEST_CASE(Timer, ScopedTimerCallback) {
  double recorded = 0.0;
  {
    ScopedTimer scoped([&recorded](double ms) { recorded = ms; });
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
  }
  ASSERT_TRUE(recorded >= 2.0);
}
