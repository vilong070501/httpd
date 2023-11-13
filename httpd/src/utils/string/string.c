#include "string.h"

#include <stdlib.h>
#include <string.h>

struct string *string_create(const char *str, size_t size)
{
    if (!str)
        return NULL;

    struct string *new_string = malloc(sizeof(struct string));
    if (!new_string)
        return NULL;

    char *buffer = malloc(size);
    if (!buffer)
    {
        free(new_string);
        return NULL;
    }

    new_string->size = size;
    new_string->data = buffer;
    for (size_t i = 0; i < size; i++)
    {
        new_string->data[i] = str[i];
    }
    return new_string;
}

int string_compare_n_str(const struct string *str1, const char *str2, size_t n)
{
    return strncmp(str1->data, str2, n);
}

void string_concat_str(struct string *str, const char *to_concat, size_t size)
{
    if (!to_concat)
        return;

    size_t old_size = str->size;
    str->size += size;
    str->data = realloc(str->data, str->size);
    for (size_t i = 0; i < size; i++)
    {
        str->data[old_size + i] = to_concat[i];
    }
}

void string_destroy(struct string *str)
{
    free(str->data);
    free(str);
}
