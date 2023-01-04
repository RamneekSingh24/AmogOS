#include "include/stdio.h"
#include "include/stdlib.h"
#include "include/string.h"

void screen() {
    printf("Yo %d\n", 123);
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
        if (c == 'P') {
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
}

int main(int argc, char **argv) {

    if (argc >= 2 && strncmp(argv[1], "showmt", 5) == 0) {
        int i = 0;
        while (1) {
            i++;
            if (i % 30000000 == 0) {
                printf("i=%d\n", i);
            }
            if (i < 0 || i > 30000000 * 8) {
                return 0;
            }
        }
    }

    print("Greetings! you started blank.c and passed the args listed below\n",
          100);

    printf("&argc: %p\n", &argc);
    printf("&argv: %p\n", &argv);
    printf("argc: %d\n", argc);
    printf("argv: %p\n", argv);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]:  %s\n", i, argv[i]);
    }

    printf("BLANK: I am going to page fault now\n");
    int x = *(int *)0x30; // page fault
    printf("x=%d\n", x);
    return 0;
}