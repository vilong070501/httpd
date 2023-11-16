#include <stdio.h>

#include "string.h"

void print_string(struct string *str)
{
    if (!str)
        return;

    for (size_t i = 0; i < str->size; i++)
    {
        printf("%c", str->data[i]);
    }
    printf("\n");
}

int main(void)
{
    struct string *new = string_create(NULL, 0);
    print_string(new);
    printf("%d\n", string_compare_n_str(new, "\0", 1));
    string_destroy(new);
    return 0;
}
