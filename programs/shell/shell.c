#include "include/stdio.h"
#include "include/stdlib.h"
#include "include/string.h"
#include "include/unistd.h"

void wait_pid_blocking(int pid) {
    while (waitpid(pid) != 0) {
    };
}

void shell(int argc, char **argv) {
    printf("Welcome to AmogOS, Sussy Baka\n");
    char buf[1024];
    while (1) {
        print("> ", 10);
        readline_terminal(buf, 100);
        if (strlen(buf) == 0) {
            continue;
        }
        if (istrncmp(buf, "clear", 5) == 0) {
            cls();
        } else if (istrncmp(buf, "tok", 3) == 0) {
            char *token = strtok(buf, " ");
            printf("Token: %s, length=%d\n", token, strlen(token));
            while (token != NULL) {
                printf("%s\n", token);
                token = strtok(NULL, " ");
            }
        } else if (istrncmp(buf, "info args", 9) == 0) {
            printf("&argc: %p\n", &argc);
            printf("&argv: %p\n", &argv);
            printf("argc: %d\n", argc);
            printf("argv: %p\n", argv);
            for (int i = 0; i < argc; i++) {
                printf("argv[%d]:  %s\n", i, argv[i]);
            }
        } else if (istrncmp(buf, "showmt", 6) == 0) {
            cls();
            printf("Showing multi tasking capabilities: Launching two procs "
                   "that print to screen counting upwards\n");
            int p1 = create_proccess("0:/blank", 2, 16, "0:/blank\0showmt\0");
            int p2 = create_proccess("0:/blank", 2, 16, "0:/blank\0showmt\0");
            if (p1 >= 0) {
                wait_pid_blocking(p1);
            }
            if (p2 >= 0) {
                wait_pid_blocking(p2);
            }
        } else {
            // start a process from the input
            char *token = strtok(buf, " ");
            // Prepare the arguments
            int argc = 1;
            int len = strlen(token) + 1;
            char *tmp = strtok(NULL, " ");
            while (tmp != NULL) {
                argc++;
                len += strlen(tmp) + 1;
                tmp = strtok(NULL, " ");
            }

            str_to_lower(token);

            char FILE_PATH[128];
            for (int i = 0; i < 128; i++) {
                FILE_PATH[i] = 0;
            }

            strcpy(FILE_PATH, "0:/");
            strncpy(FILE_PATH + 3, token, 120);
            int res = create_proccess(FILE_PATH, argc, len, buf);
            if (res < 0) {
                printf("Command not found: %s\n", token);
            } else {
                // printf("Process created with pid=%d, waiting for it now\n",
                //        res);
                wait_pid_blocking(res);
            }
        }
    }
}

int main(int argc, char **argv) {
    shell(argc, argv);
    while (1) {
    };
    return 0;
}
