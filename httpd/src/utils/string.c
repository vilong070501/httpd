#include "string.h"

#include <stdlib.h>
#include <string.h>

struct string *string_create(const char *str, size_t size)
{
    struct string *new_string = malloc(sizeof(struct string));
    if (!new_string)
        return NULL;

    if (!str)
    {
        new_string->data = NULL;
        new_string->size = 0;
        return new_string;
    }

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
    for (size_t i = 0; i < n; i++)
    {
        if (str1->data[i] != str2[i])
            return (str1->data[i] > str2[i]) ? 1 : -1;
    }
    return 0;
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
    if (!str)
        return;
    free(str->data);
    free(str);
}
