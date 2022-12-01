#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include <stdint.h>
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"



uint16_t* video_mem = 0;
uint16_t curr_y = 0;
uint16_t curr_x = 0;

uint16_t make_char(char c, char color) {
    return (color << 8) | c;  // Little Endian
}

void screen_init() {
    video_mem = (uint16_t*)(0xB8000);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[VGA_WIDTH * y + x] = make_char(' ', 0);
        }
    }
}

void screen_put_char(int x, int y, char c, char color) {

    video_mem[y * VGA_WIDTH + x] = make_char(c, color);
    
}


void screen_write_char(char c, char color) {
    if (c == '\n') {
        curr_y++;
        curr_x = 0;
        return;
    }
    screen_put_char(curr_x, curr_y, c, color);
    curr_x++;
    if (curr_x == VGA_WIDTH) {
        curr_y++;
        curr_x = 0;
    }
}

void print(char* str) {
    char *c = str;
    while (*c) {
        screen_write_char(*c, 15);
        c++;
    }
}


static struct page_table_directory* kernel_page_table_dir = NULL;

void kernel_main() {
    screen_init();
    print("bb study\nbbstudy");

    kheap_init();

    disk_search_init();

    idt_init();

    kernel_page_table_dir = create_paging_dir(PAGE_WRITEABLE | PAGING_ENABLED | PAGE_ACCESS_FROM_ALL);
    paging_switch(get_page_tables(kernel_page_table_dir));


    enable_paging();



    enable_interrupts();

    print("\nola\n");
    
    

    int k = 0;
    int x = 1 / k;

    char* c = "hello\n";

    if (x == 1) {
        c = "world\n";
    }

    print(c);




}
