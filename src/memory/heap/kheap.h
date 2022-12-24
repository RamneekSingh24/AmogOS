#ifndef KHEAP_H
#define KHEAP_H

#include "config.h"
#include <stddef.h>
#include <stdint.h>

// Heap interface for the kernel
// Simple block based implementation

#define KHEAP_BLOCK_SIZE 4096            // 4 KB
#define KERNEL_HEAP_SIZE_BYTES 104857600 // 100 MB (static heap size for now)

/*
    x86 memory map(https://wiki.osdev.org/Memory_Map_(x86)):

    start 	    end 	    size 	  description 	type
                Real mode address space (the first MiB)
    0x00000000 	0x000003FF 	1 KiB 	Real Mode IVT (Interrupt Vector Table)
   unusable in real mode 	640 KiB RAM ("Low memory") 0x00000400
   0x000004FF 	256 bytes 	BDA (BIOS data area)
    0x00000500 	0x00007BFF 	almost 30 KiB 	Conventional memory usable
   memory 0x00007C00 	0x00007DFF 	512 bytes 	Your OS BootSector
    0x00007E00 	0x0007FFFF 	480.5 KiB 	Conventional memory
    0x00080000 	0x0009FFFF 	128 KiB 	EBDA (Extended BIOS Data Area)
   partially used by the EBDA
    0x000A0000 	0x000BFFFF 	128 KiB 	Video display memory 	hardware
   mapped 	384 KiB System / Reserved ("Upper Memory") 0x000C0000
   0x000C7FFF 	32 KiB (typically) 	Video BIOS 	ROM and hardware mapped
   / Shadow RAM 0x000C8000 	0x000EFFFF 	160 KiB (typically) BIOS
   Expansions 0x000F0000 	0x000FFFFF 	64 KiB


    Extended memory


    start 	end 	size 	region/exception 	description
    High Memory
    0x00100000 	0x00EFFFFF 	0x00E00000 (14 MiB) 	RAM -- free for use (if
   it exists) 	Extended memory 1, 2
    0x00F00000 	0x00FFFFFF 	0x00100000 (1 MiB) 	Possible memory mapped
   hardware 	ISA Memory Hole 15-16MB 3 0x01000000 	 ???????? ????????
   (whatever exists) 	RAM -- free for use 	More Extended memory 1
    0xC0000000 (sometimes, depends on motherboard and devices) 	0xFFFFFFFF
   0x40000000 (1 GiB) 	various (typically reserved for memory mapped devices)
   Memory mapped PCI devices, PnP NVRAM?, IO APIC/s, local APIC/s, BIOS, ...
    0x0000000100000000 (possible memory above 4 GiB) 	 ????????????????
   ???????????????? (whatever exists) 	RAM -- free for use (PAE/64bit)
   More Extended memory 1
    ???????????????? 	 ???????????????? 	 ???????????????? 	Possible
   memory mapped hardware 	Potentially usable for memory mapped PCI devices
   in modern hardware (but typically not, due to backward compatibility)
*/

#define KHEAP_ADDR 0x01000000

// For now we are using some memory from the real mode address space
// for the kheap table
// later we can keep the table at top of the externded memory(0x01000000)
// and start heap from after it.
#define KHEAP_TABLE_ADDR 0x00007E00 // (4 KiB)

#define KHEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define KHEAP_BLOCK_TABLE_ENTRY_FREE 0x00

#define KHEAP_BLOCK_HAS_NEXT 0b10000000
#define KHEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned char KHEAP_BLOCK_TABLE_ENTRY;

struct KHEAP_ENTRY_TABLE {
    KHEAP_BLOCK_TABLE_ENTRY *entries;
    size_t num_entries;
};

struct KHEAP {
    struct KHEAP_ENTRY_TABLE *entry_table;
    void *kheap_physical_start_addr;
};

void *kmalloc(size_t size);
void *kzalloc(size_t size);
int kfree(void *ptr);

int kheap_init();

void kheap_test();

#endif