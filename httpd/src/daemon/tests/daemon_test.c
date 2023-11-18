#include <criterion/criterion.h>

TestSuite(daemon);

Test(daemon, dummy)
{
    cr_assert(1);
}
