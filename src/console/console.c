#include "console.h"
#include <stdint.h>


uint16_t* video_mem = 0;
uint16_t curr_y = 0;
uint16_t curr_x = 0;

uint16_t make_char(char c, char color) {
    return (color << 8) | c;  // Little Endian
}

void console_init() {
    video_mem = (uint16_t*)(0xB8000);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[VGA_WIDTH * y + x] = make_char(' ', 0);
        }
    }
}

void console_put_char(int x, int y, char c, char color) {

    video_mem[y * VGA_WIDTH + x] = make_char(c, color);
    
}


void console_write_char(char c, char color) {
    if (c == '\n') {
        curr_y++;
        curr_x = 0;
        return;
    }
    console_put_char(curr_x, curr_y, c, color);
    curr_x++;
    if (curr_x == VGA_WIDTH) {
        curr_y++;
        curr_x = 0;
    }
}

void print(char* str) {
    char *c = str;
    while (*c) {
        console_write_char(*c, 15);
        c++;
    }
}


void println(char* str) {
    print(str);
    print("\n");
}

void print_int(int x) {
    if (x < 0) {
        print("-");
        x = -x;
    }
    int cx  = x;
    int num_digits = 0;

    while(x > 0) {
        num_digits++;
        x /= 10;
    }
    if (num_digits == 0) {
        print("0");
        return;
    }
    x = cx;
    int p10 = 1;
    for (int i = 0; i < num_digits; i++) {
        p10 *= 10;
    }
    for (int i = 0; i < num_digits; i++) {
        p10 /= 10;
        int d = x / p10;
        console_write_char('0' + d, 15);
        x -= d * p10;
    }

}