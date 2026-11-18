#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "karatsuba/terminal_color.hpp"

namespace km::testing {

struct TestFailure : public std::exception {
  std::string message;
  TestFailure(std::string msg) : message(std::move(msg)) {}
  const char* what() const noexcept override { return message.c_str(); }
};

struct TestCase {
  std::string suite;
  std::string name;
  std::function<void()> body;
};

inline std::vector<TestCase>& getRegistry() {
  static std::vector<TestCase> registry;
  return registry;
}

inline void registerTest(std::string suite, std::string name, std::function<void()> body) {
  getRegistry().push_back({std::move(suite), std::move(name), std::move(body)});
}

#define TEST_CASE(suite_name, test_name)                                        \
  static void test_##suite_name##_##test_name();                                \
  namespace {                                                                   \
  struct Reg_##suite_name##_##test_name {                                       \
    Reg_##suite_name##_##test_name() {                                          \
      ::km::testing::registerTest(#suite_name, #test_name,                      \
                                  test_##suite_name##_##test_name);             \
    }                                                                           \
  } instance_##suite_name##_##test_name;                                        \
  }                                                                             \
  static void test_##suite_name##_##test_name()

#define ASSERT_TRUE(cond)                                                       \
  do {                                                                          \
    if (!(cond)) {                                                              \
      std::ostringstream oss;                                                   \
      oss << "Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__; \
      throw ::km::testing::TestFailure(oss.str());                              \
    }                                                                           \
  } while (false)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b)                                                         \
  do {                                                                          \
    if (!((a) == (b))) {                                                        \
      std::ostringstream oss;                                                   \
      oss << "Assertion failed: (" #a " == " #b ") [" << (a) << " != " << (b)  \
          << "] at " << __FILE__ << ":" << __LINE__;                            \
      throw ::km::testing::TestFailure(oss.str());                              \
    }                                                                           \
  } while (false)

#define ASSERT_NE(a, b)                                                         \
  do {                                                                          \
    if ((a) == (b)) {                                                           \
      std::ostringstream oss;                                                   \
      oss << "Assertion failed: (" #a " != " #b ") [" << (a) << " == " << (b)  \
          << "] at " << __FILE__ << ":" << __LINE__;                            \
      throw ::km::testing::TestFailure(oss.str());                              \
    }                                                                           \
  } while (false)

inline int runAllTests() {
  const bool color = color::isColorSupported();
  auto& tests = getRegistry();

  std::cout << color::bold(color) << color::cyan(color)
            << "========================================================\n"
            << "              Running Test Suite (" << tests.size() << " tests)\n"
            << "========================================================\n"
            << color::reset(color);

  std::size_t passed = 0;
  std::size_t failed = 0;

  for (const auto& test : tests) {
    std::cout << "  [ RUN      ] " << test.suite << "." << test.name << std::flush;
    try {
      test.body();
      std::cout << "\r" << color::green(color) << "  [       OK ] "
                << color::reset(color) << test.suite << "." << test.name << "\n";
      ++passed;
    } catch (const TestFailure& e) {
      std::cout << "\r" << color::red(color) << "  [  FAILED  ] "
                << color::reset(color) << test.suite << "." << test.name << "\n"
                << "               " << color::red(color) << e.what()
                << color::reset(color) << "\n";
      ++failed;
    } catch (const std::exception& e) {
      std::cout << "\r" << color::red(color) << "  [  FAILED  ] "
                << color::reset(color) << test.suite << "." << test.name << "\n"
                << "               Unexpected exception: " << e.what() << "\n";
      ++failed;
    }
  }

  std::cout << color::bold(color) << "\n--------------------------------------------------------\n"
            << "Tests Passed: " << color::green(color) << passed << color::reset(color)
            << " | Tests Failed: " << (failed > 0 ? color::red(color) : color::green(color))
            << failed << color::reset(color) << " | Total: " << tests.size() << "\n"
            << "--------------------------------------------------------\n"
            << color::reset(color);

  return (failed == 0) ? 0 : 1;
}

} // namespace km::testing
