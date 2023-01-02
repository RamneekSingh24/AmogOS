#include <os.h>

int main(int argc, char** argv) {
    print("Greetings! you are inside a terminal written in C\n", 100);
    while (1) {
        int c = get_key();
        if (c == 0) {
            continue;
        }
        put_char(c);
    };
    return 0;
}