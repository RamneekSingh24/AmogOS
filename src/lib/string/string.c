#include "string.h"

int strlen(const char *str) {
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

int strnlen(const char *str, int maxlen) {
    int i = 0;
    while (str[i] != '\0' && i < maxlen) {
        i++;
    }
    return i;
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }