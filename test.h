#pragma once

#include <random>

using test_func_t = void (*)();

void register_test(char const *, test_func_t);

void run_tests();

std::mt19937_64 & test_rng();

#define PASTE_IMPL(x, y) x##y

#define PASTE(x, y) PASTE_IMPL(x, y)

#define TEST(name)                                                                                 \
    static void PASTE(test_, __LINE__)();                                                          \
    __attribute__((constructor)) static void PASTE(insert_test_, __LINE__)()                       \
    {                                                                                              \
        register_test(name, &PASTE(test_, __LINE__));                                              \
    }                                                                                              \
    static void PASTE(test_, __LINE__)()
