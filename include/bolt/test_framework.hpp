#ifndef BOLT_TEST_FRAMEWORK_HPP
#define BOLT_TEST_FRAMEWORK_HPP

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <sstream>
#include <exception>
#include <chrono>

namespace bolt {
namespace test {

class TestFailure : public std::exception {
public:
    TestFailure(const std::string& message, const std::string& file, int line)
        : message_(message), file_(file), line_(line) {
        std::stringstream ss;
        ss << file << ":" << line << " - " << message;
        full_message_ = ss.str();
    }

    const char* what() const noexcept override {
        return full_message_.c_str();
    }

    const std::string& getMessage() const { return message_; }
    const std::string& getFile() const { return file_; }
    int getLine() const { return line_; }

private:
    std::string message_;
    std::string file_;
    int line_;
    std::string full_message_;
};

class TestSuite {
public:
    struct TestCase {
        std::string name;
        std::function<void()> test_func;
        std::string suite_name;
    };

    struct TestResult {
        std::string name;
        bool passed;
        std::string error_message;
        double duration_ms;
        std::string suite_name;
    };

    static TestSuite& getInstance() {
        static TestSuite instance;
        return instance;
    }

    void addTest(const std::string& suite_name, const std::string& test_name, std::function<void()> test_func) {
        tests_.push_back({test_name, test_func, suite_name});
    }

    int runAllTests(bool verbose = true) {
        std::vector<TestResult> results;
        int passed = 0, failed = 0;

        if (verbose) {
            std::cout << "Running " << tests_.size() << " tests...\n" << std::endl;
        }

        for (const auto& test : tests_) {
            if (verbose) {
                std::cout << "[" << test.suite_name << "] " << test.name << " ... ";
                std::cout.flush();
            }

            auto start = std::chrono::high_resolution_clock::now();
            TestResult result;
            result.name = test.name;
            result.suite_name = test.suite_name;

            try {
                test.test_func();
                result.passed = true;
                passed++;
                if (verbose) {
                    std::cout << "PASS" << std::endl;
                }
            } catch (const TestFailure& e) {
                result.passed = false;
                result.error_message = e.what();
                failed++;
                if (verbose) {
                    std::cout << "FAIL" << std::endl;
                    std::cout << "  " << e.what() << std::endl;
                }
            } catch (const std::exception& e) {
                result.passed = false;
                result.error_message = "Unexpected exception: " + std::string(e.what());
                failed++;
                if (verbose) {
                    std::cout << "ERROR" << std::endl;
                    std::cout << "  " << result.error_message << std::endl;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            result.duration_ms = duration.count() / 1000.0;

            results_.push_back(result);
        }

        if (verbose) {
            std::cout << std::endl;
            printSummary(passed, failed);
        }

        return failed;
    }

    int runTestsInSuite(const std::string& suite_name, bool verbose = true) {
        std::vector<TestResult> results;
        int passed = 0, failed = 0;
        int total_in_suite = 0;

        // Count tests in suite
        for (const auto& test : tests_) {
            if (test.suite_name == suite_name) {
                total_in_suite++;
            }
        }

        if (total_in_suite == 0) {
            if (verbose) {
                std::cout << "No tests found in suite: " << suite_name << std::endl;
            }
            return 0;
        }

        if (verbose) {
            std::cout << "Running " << total_in_suite << " tests in suite '" << suite_name << "'...\n" << std::endl;
        }

        for (const auto& test : tests_) {
            if (test.suite_name != suite_name) continue;

            if (verbose) {
                std::cout << "[" << test.suite_name << "] " << test.name << " ... ";
                std::cout.flush();
            }

            auto start = std::chrono::high_resolution_clock::now();
            TestResult result;
            result.name = test.name;
            result.suite_name = test.suite_name;

            try {
                test.test_func();
                result.passed = true;
                passed++;
                if (verbose) {
                    std::cout << "PASS" << std::endl;
                }
            } catch (const TestFailure& e) {
                result.passed = false;
                result.error_message = e.what();
                failed++;
                if (verbose) {
                    std::cout << "FAIL" << std::endl;
                    std::cout << "  " << e.what() << std::endl;
                }
            } catch (const std::exception& e) {
                result.passed = false;
                result.error_message = "Unexpected exception: " + std::string(e.what());
                failed++;
                if (verbose) {
                    std::cout << "ERROR" << std::endl;
                    std::cout << "  " << result.error_message << std::endl;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            result.duration_ms = duration.count() / 1000.0;

            results_.push_back(result);
        }

        if (verbose) {
            std::cout << std::endl;
            printSummary(passed, failed);
        }

        return failed;
    }

    const std::vector<TestResult>& getResults() const {
        return results_;
    }

    void listTests() const {
        std::cout << "Available tests:\n" << std::endl;
        std::string current_suite = "";
        for (const auto& test : tests_) {
            if (test.suite_name != current_suite) {
                current_suite = test.suite_name;
                std::cout << "Suite: " << current_suite << std::endl;
            }
            std::cout << "  " << test.name << std::endl;
        }
        std::cout << std::endl;
        std::cout << "Total: " << tests_.size() << " tests" << std::endl;
    }

private:
    std::vector<TestCase> tests_;
    std::vector<TestResult> results_;

    void printSummary(int passed, int failed) {
        std::cout << "Test Results:" << std::endl;
        std::cout << "  Passed: " << passed << std::endl;
        std::cout << "  Failed: " << failed << std::endl;
        std::cout << "  Total:  " << (passed + failed) << std::endl;

        if (failed == 0) {
            std::cout << std::endl << "All tests passed!" << std::endl;
        } else {
            std::cout << std::endl << failed << " test(s) failed." << std::endl;
        }
    }
};

// Test registration helper
class TestRegistrar {
public:
    TestRegistrar(const std::string& suite_name, const std::string& test_name, std::function<void()> test_func) {
        TestSuite::getInstance().addTest(suite_name, test_name, test_func);
    }
};

// Assertion macros
#define BOLT_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            throw bolt::test::TestFailure("Assertion failed: " #condition, __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::stringstream ss; \
            ss << "Expected '" << (expected) << "', but got '" << (actual) << "'"; \
            throw bolt::test::TestFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::stringstream ss; \
            ss << "Expected values to be different, but both were '" << (actual) << "'"; \
            throw bolt::test::TestFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_TRUE(condition) BOLT_ASSERT(condition)

#define BOLT_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw bolt::test::TestFailure("Expected false, but got true: " #condition, __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            throw bolt::test::TestFailure("Expected null pointer, but got non-null", __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw bolt::test::TestFailure("Expected non-null pointer, but got null", __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_THROWS(exception_type, code) \
    do { \
        bool threw_expected = false; \
        try { \
            code; \
        } catch (const exception_type&) { \
            threw_expected = true; \
        } catch (...) { \
            throw bolt::test::TestFailure("Expected " #exception_type " but caught different exception", __FILE__, __LINE__); \
        } \
        if (!threw_expected) { \
            throw bolt::test::TestFailure("Expected " #exception_type " but no exception was thrown", __FILE__, __LINE__); \
        } \
    } while (0)

// Test case registration macro
#define BOLT_TEST(suite_name, test_name) \
    void test_##suite_name##_##test_name(); \
    static bolt::test::TestRegistrar registrar_##suite_name##_##test_name(#suite_name, #test_name, test_##suite_name##_##test_name); \
    void test_##suite_name##_##test_name()

} // namespace test
} // namespace bolt

#endif // BOLT_TEST_FRAMEWORK_HPP