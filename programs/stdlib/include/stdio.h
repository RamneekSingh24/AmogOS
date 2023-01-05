#ifndef STDIO_H
#define STDIO_H

void print(const char *str, int len);
void put_char(int c);
int get_key();
int printf(const char *fmt, ...);
void readline_terminal(char *buf, int max_len);
void cls();

#endif