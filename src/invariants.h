#ifndef INVARIANTS_H
#define INVARIANTS_H
#include "kernel.h"
#include <stdbool.h>

static bool no_page_sharing = true;
static bool kernel_mapped_in_user_va = true;
static bool interrupt_handler_cli_always = true;
static bool single_cpu = true;
static bool kernel_uses_less_than_128mb_phy_adders = true;

static void assert_no_page_sharing() {
    if (!no_page_sharing) {
        panic("Using no page sharing invariant");
    }
}

static void assert_kernel_mapped_in_user_va() {
    if (!kernel_mapped_in_user_va) {
        panic("Using kernel mapped in user va invariant");
    }
}

static void assert_interrupt_handler_cli_always() {
    if (!interrupt_handler_cli_always) {
        panic("Using interrupt handler cli always invariant");
    }
}

static void assert_single_cpu() {
    if (!single_cpu) {
        panic("Using single cpu invariant");
    }
}

static void assert_all_of_kernel_uses_less_than_128mb_phy_adders() {
    if (!kernel_uses_less_than_128mb_phy_adders) {
        panic("Using all of kernel uses less than 2gb phy adders invariant");
    }
}

#endif