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

void strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void strncpy(char *dst, const char *src, int maxlen) {
    int i = 0;
    while (i < maxlen && src[i]) {
        dst[i] = src[i];
        i++;
    }
    if (i == maxlen && i > 0) {
        dst[i - 1] = '\0';
    } else if (i < maxlen) {
        dst[i] = '\0';
    } else if (i == 0 && maxlen > 0) {
        dst[i] = '\0';
    }
}

int strncmp(const char *str1, const char *str2, int maxlen) {
    int i = 0;
    while (i < maxlen && str1[i] && str2[i]) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        i++;
    }
    if (i == maxlen) {
        return 0;
    } else if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    } else if (str1[i] == '\0') {
        return -1;
    } else {
        return 1;
    }
}

int strnlen_terminator(const char *str, int maxlen, char terminator) {
    int i = 0;
    while (i < maxlen && str[i] != '\0' && str[i] != terminator) {
        i++;
    }
    return i;
}

char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

int istrncmp(const char *str1, const char *str2, int maxlen) {
    int i = 0;
    while (i < maxlen && str1[i] && str2[i]) {
        if (to_lower(str1[i]) != to_lower(str2[i])) {
            return str1[i] - str2[i];
        }
        i++;
    }
    if (i == maxlen) {
        return 0;
    } else if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    } else if (str1[i] == '\0') {
        return -1;
    } else {
        return 1;
    }
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }