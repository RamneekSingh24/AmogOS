#ifndef CONFIG_H
#define CONFIG_H

#define NUM_INTERRUPTS 512       // 512 interrupts supported by intel's i386
#define KERNEL_CODE_SEGMENT 0x08 // 0x08 is the code segment for the kernel
#define KERNEL_DATA_SEGMENT 0x10 // 0x10 is the data segment for the kernel

#define MASTER_PIC_PORT 0x20
#define MASTER_PIC_INTR_ACK 0x20

#define N_CPU_MAX 32

#define DISK_SECTOR_SIZE 512

#define FS_MAX_PATH_LEN 108

#define MAX_FILESYSTEMS 8
#define MAX_FILE_DESCRIPTORS 1024

#define TOTAL_GDT_SEGS 6

#define KHEAP_SAFE_BOUNDARY                                                    \
    0x8000000 // INVARIANT: Kernel will not use physical addresses beyond this
              // Safe to map processes memory beyond this

#define DEFAULT_USER_PROG_ENTRY (KHEAP_SAFE_BOUNDARY + 0x400000) // 0x8400000
#define DEFAULT_USER_DATA_SEGMENT 0x23
#define DEFAULT_USER_CODE_SEGMENT 0x1B
#define DEFAULT_USER_STACK_START (KHEAP_SAFE_BOUNDARY + 0x3FF000) // 0x83ff000
#define DEFAULT_USER_STACK_SIZE (1024 * 16)                       // 16 KB
#define DEFAULT_USER_STACK_END                                                 \
    (DEFAULT_USER_STACK_START - DEFAULT_USER_STACK_SIZE)

#define MAX_PROCS 10

#define PROCESS_VMEM_VMEM_MAX_BLOCK_COUNT 10
#define PROCESS_MAX_OPEN_FILES 10

#define NUM_SYS_CALLS 64

#endif