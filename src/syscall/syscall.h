#ifndef SYSCALL_H
#define SYSCALL_H

enum SYS_CALLS {
    SYS_CALL0_HELLO_WORLD_SUM,
    SYS_CALL1_PRINT,
    SYS_CALL2_GET_CHAR,
    SYS_CALL3_PUT_CHAR,
};

void register_syscalls();

#endif