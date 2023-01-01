#ifndef CONSOLE_H
#define CONSOLE_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

void print(char *str);
void println(char *str);
void print_int(int x);
void console_init();
void printn(char *str, int n);
void print_char(char c);

#endif