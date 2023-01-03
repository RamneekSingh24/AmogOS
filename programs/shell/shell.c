#include "include/stdio.h"
#include "include/stdlib.h"
#include "include/string.h"

void shell() {
    print("Welcome to AmogOS, Sussy Baka\n", 100);
    char buf[1024];
    while (1) {
        print("> ", 10);
        readline_terminal(buf, 100);
        if (istrncmp(buf, "clear", 5) == 0) {
            cls();
        }
        if (istrncmp(buf, "tok", 3) == 0) {
            char *token = strtok(buf, " ");
            while (token != NULL) {
                printf("%s\n", token);
                token = strtok(NULL, " ");
            }
        }
    }
}

int main(int argc, char **argv) {
    shell();
    while (1) {
    };
    return 0;
}
