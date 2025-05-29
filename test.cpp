#include <cstdio>
#include <vector>

#include "test.h"

struct test
{
    char const * name;
    test_func_t func;
};

struct test_registry
{
    void insert(char const * name, test_func_t func)
    {
        tests.push_back({name, func});
    }

    void run()
    {
        for (auto const & [name, func] : tests) {
            printf("running test '%s'\n", name);
            func();
        }
    }

    static test_registry & instance()
    {
        static test_registry the_instance;
        return the_instance;
    }

private:
    std::vector<test> tests;
};

void register_test(char const * name, test_func_t func)
{
    test_registry::instance().insert(name, func);
}

void run_tests()
{
    test_registry::instance().run();
}
