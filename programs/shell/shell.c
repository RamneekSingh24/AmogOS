#include "include/stdio.h"
#include "include/stdlib.h"
#include "include/string.h"
#include "include/unistd.h"

void shell() {
    printf("Welcome to AmogOS, Sussy Baka\n");
    char buf[1024];
    while (1) {
        print("> ", 10);
        readline_terminal(buf, 100);
        if (istrncmp(buf, "clear", 5) == 0) {
            cls();
        }
        else if (istrncmp(buf, "tok", 3) == 0) {
            char *token = strtok(buf, " ");
            printf("Token: %s, length=%d\n", token, strlen(token));
            while (token != NULL) {
                printf("%s\n", token);
                token = strtok(NULL, " ");
            }
        }
        else {
            // start a process from the input
            char* token = strtok(buf, " ");
            char* tmp = token;
            // TODO: For now, we don't support arguments so we just skip them
            while (tmp != NULL) {
                tmp = strtok(NULL, " ");
            }
            str_to_lower(token);
            char FILE_PATH[128];
            for (int i = 0; i < 128; i++) {
                FILE_PATH[i] = 0;
            }
            strcpy(FILE_PATH, "0:/");
            strncpy(FILE_PATH + 3, token, 120);
            int res = create_proccess(FILE_PATH, 0, NULL);
            if (res != 0) {
                printf("Command not found: %s\n", token);
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
