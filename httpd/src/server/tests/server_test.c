#include <criterion/criterion.h>

TestSuite(server);

Test(server, dummy)
{
    cr_assert(1);
}
