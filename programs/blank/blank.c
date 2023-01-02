#include "include/stdio.h"
#include "include/stdlib.h"

int main(int argc, char **argv) {
    print("Greetings! you are inside a terminal written in C\n", 100);

    while (1) {
        int c = get_key();
        if (c == 0) {
            continue;
        }
        put_char(c);
        if (c == 'M') {
            void *ptr = malloc(28);
            if (ptr == 0) {
                print("malloc failed\n", 100);
            } else {
                print("malloc success\n", 100);
                for (int i = 0; i < 28; i++) {
                    *(char *)(ptr + i) = 'a' + i;
                    if (i == 27) {
                        *(char *)(ptr + i) = 0;
                    }
                }
                print((char *)ptr, 100);
                put_char('\n');
            }
        }
        if (c == 'A') {
            *(int *)0x100000 = 32; // crash
        }
        if (c == 'I') {
            char buf[12];
            int x = 123456789;
            itoa(x, buf);
            print(buf, 100);
            print("19891213", 100);
            x = atoi("19891213");
            char buf2[12];
            itoa(x, buf2);
            put_char('\n');
            print(buf2, 100);
            put_char('\n');
        }
    };
    return 0;
}