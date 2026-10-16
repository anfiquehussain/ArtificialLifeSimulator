#pragma once

#include <iostream>
#include <cassert>

namespace alife::test {

inline int totalTests = 0;
inline int failedTests = 0;

inline void reportSummary() {
    std::cout << "\n========================================\n";
    std::cout << "Tests run: " << totalTests << ", Passed: " << (totalTests - failedTests) 
              << ", Failed: " << failedTests << "\n";
    std::cout << "========================================\n";
}

} // namespace alife::test

#define ALIFE_TEST_ASSERT(condition, msg) \
    do { \
        ++alife::test::totalTests; \
        if (!(condition)) { \
            ++alife::test::failedTests; \
            std::cerr << "FAIL: [" << __FILE__ << ":" << __LINE__ << "] " << msg << "\n"; \
        } else { \
            std::cout << "PASS: " << msg << "\n"; \
        } \
    } while (false)
