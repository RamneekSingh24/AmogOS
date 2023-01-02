#ifndef INVARIANTS_H
#define INVARIANTS_H
#include "kernel.h"
#include <stdbool.h>

bool no_page_sharing = true;
bool kernel_mapped_in_user_va = true;
bool interrupt_handler_cli_always = true;
bool single_cpu = true;
bool kernel_uses_less_than_128mb_phy_adders = true;

void assert_no_page_sharing() {
    if (!no_page_sharing) {
        panic("Using no page sharing invariant");
    }
}

void assert_kernel_mapped_in_user_va() {
    if (!kernel_mapped_in_user_va) {
        panic("Using kernel mapped in user va invariant");
    }
}

void assert_interrupt_handler_cli_always() {
    if (!interrupt_handler_cli_always) {
        panic("Using interrupt handler cli always invariant");
    }
}

void assert_single_cpu() {
    if (!single_cpu) {
        panic("Using single cpu invariant");
    }
}

void assert_all_of_kernel_uses_less_than_128mb_phy_adders() {
    if (!kernel_uses_less_than_128mb_phy_adders) {
        panic("Using all of kernel uses less than 2gb phy adders invariant");
    }
}

#endif