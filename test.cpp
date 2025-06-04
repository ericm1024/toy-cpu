#include "test.h"

#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <vector>

static logger logger{__FILE__};

struct test
{
    char const * name;
    test_func_t func;
};

// TODO: thread safety
static std::mt19937_64 global_rng;

struct test_registry
{
    void insert(char const * name, test_func_t func)
    {
        tests.push_back({name, func});
    }

    void run()
    {
        for (auto const & [name, func] : tests) {
            uint32_t seed = get_seed();
            global_rng.seed(seed);
            logger.info("running test '{}' rng seed {} ", name, seed);
            func();
        }
    }

    static test_registry & instance()
    {
        static test_registry the_instance;
        return the_instance;
    }

private:
    uint32_t get_seed()
    {
        if (char const * env = getenv("TEST_RNG_SEED")) {
            return atoi(env);
        }
        return rng();
    }

    std::mt19937_64 rng{std::random_device{}()};
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

std::mt19937_64 & test_rng()
{
    return global_rng;
}
