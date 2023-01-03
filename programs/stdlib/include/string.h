#ifndef STRING_H
#define STRING_H

#include <stdbool.h>

int strlen(const char *str);
int strnlen(const char *str, int maxlen);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, int maxlen);
int strncmp(const char *str1, const char *str2, int maxlen);
int strnlen_terminator(const char *str, int maxlen, char terminator);
int istrncmp(const char *str1, const char *str2, int maxlen);
char to_lower(char c);
void str_to_lower(char *str);
bool is_digit(char c);
char *strtok(char *str, const char *delimiters);

#endif