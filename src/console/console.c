#include "console.h"
#include <stdint.h>

uint16_t *video_mem = 0;
uint16_t curr_y = 0;
uint16_t curr_x = 0;

uint16_t make_char(char c, char color) {
    return (color << 8) | c; // Little Endian
}

void console_init() {
    video_mem = (uint16_t *)(0xB8000);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[VGA_WIDTH * y + x] = make_char(' ', 0);
        }
    }
}

void console_put_char(int x, int y, char c, char color) {
    video_mem[y * VGA_WIDTH + x] = make_char(c, color);
}

void console_backspace() {
    if (curr_x == 0 && curr_y == 0) {
        return;
    }
    if (curr_x == 0) {
        curr_y--;
        curr_x = VGA_WIDTH - 1;
    } else {
        curr_x--;
    }
    console_put_char(curr_x, curr_y, ' ', 0);
}

void console_write_char(char c, char color) {
    if (c == '\n') {
        curr_y++;
        curr_x = 0;
        return;
    }
    if (c == 0x08) {
        console_backspace();
        return;
    }

    console_put_char(curr_x, curr_y, c, color);
    curr_x++;
    if (curr_x == VGA_WIDTH) {
        curr_y++;
        curr_x = 0;
    }
}

void print(char *str) {
    char *c = str;
    while (*c) {
        console_write_char(*c, 15);
        c++;
    }
}

void printn(char *str, int n) {
    for (int i = 0; i < n; i++) {
        if (str[i] == '\0') {
            break;
        }
        console_write_char(str[i], 15);
    }
}

void print_char(char c) { console_write_char(c, 15); }

void println(char *str) {
    print(str);
    print("\n");
}

#define MAX_INT_DIGITS 12 // 32 bit int max ~(2e9) = 10 digits + sign + null
void print_int(int x) {
    if (x < 0) {
        print("-");
        x = -x;
    }
    int cx = x;
    int num_digits = 0;

    while (x > 0) {
        num_digits++;
        x /= 10;
    }
    if (num_digits == 0) {
        print("0");
        return;
    }
    x = cx;
    char buf[MAX_INT_DIGITS];
    for (int i = 0; i < num_digits; i++) {
        buf[i] = '0' + x % 10;
        x /= 10;
    }

    for (int j = num_digits - 1; j >= 0; j--) {
        console_write_char(buf[j], 15);
    }
}

void clear_screen() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[VGA_WIDTH * y + x] = make_char(' ', 0);
        }
    }
    curr_x = 0;
    curr_y = 0;
}