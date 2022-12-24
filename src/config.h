#ifndef CONFIG_H
#define CONFIG_H

#define NUM_INTERRUPTS 512       // 512 interrupts supported by intel's i386
#define KERNEL_CODE_SEGMENT 0x08 // 0x08 is the code segment for the kernel
#define KERNEL_DATA_SEGMENT 0x10 // 0x10 is the data segment for the kernel

#define MASTER_PIC_PORT 0x20
#define MASTER_PIC_INTR_ACK 0x20

#define N_CPU_MAX 32

#define DISK_SECTOR_SIZE 512

#endif