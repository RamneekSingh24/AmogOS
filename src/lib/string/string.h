#ifndef STRING_H
#define STRING_h

#include <stdbool.h>

int strlen(const char *str);
int strnlen(const char *str, int maxlen);
bool is_digit(char c);

#endif