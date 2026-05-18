#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& testRegistry() {
    static std::vector<TestCase> r;
    return r;
}

struct TestReg {
    TestReg(const std::string& name, std::function<void()> fn) {
        testRegistry().push_back({name, std::move(fn)});
    }
};

#define ADD_TEST(name) \
    static void name##_test(); \
    static TestReg _reg_##name(#name, []() { \
        std::cout << "  " << #name << " ... "; \
        name##_test(); \
        std::cout << "OK" << std::endl; \
    }); \
    static void name##_test()

#define ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "\n  FAIL: " << #cond << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        std::exit(1); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (_a != _b) { \
        std::cerr << "\n  FAIL: " << #a << " == " << #b \
                  << " (" << _a << " != " << _b << ") " \
                  << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
} while(0)
