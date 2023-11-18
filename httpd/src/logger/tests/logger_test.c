#include <criterion/criterion.h>

TestSuite(logger);

Test(logger, dummy)
{
    cr_assert(1);
}
