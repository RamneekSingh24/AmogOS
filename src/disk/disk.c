#include "disk.h"
#include "config.h"
#include "console/console.h"
#include "io/io.h"
#include "memory/memory.h"
#include "status.h"

struct disk disk;

void disk_init() {
    memset(&disk, 0, sizeof(struct disk));
    disk.type = DISK_TYPE_REAL;
    disk.sector_size = DISK_SECTOR_SIZE;
}

struct disk *get_disk(int index) {
    if (index != 0) {
        print("ERROR: non zero disk index not supported yet\n");
        return NULL;
    }
    return &disk;
}

int disk_read_sectors(struct disk *idisk, int start_lba, int count,
                      void *buffer) {
    if (idisk != &disk) {
        print("ERROR: non zero disk index not supported yet\n");
        return -STATUS_IO_ERROR;
    }

    port_io_out_byte(0x1F6, 0xE0 | ((start_lba >> 24)));
    port_io_out_byte(0x1F2, count);
    port_io_out_byte(0x1F3, (unsigned char)(start_lba & 0xFF));
    port_io_out_byte(0x1F4, (unsigned char)(start_lba >> 8));
    port_io_out_byte(0x1F5, (unsigned char)(start_lba >> 8));
    port_io_out_byte(0x1F7, 0x20);

    unsigned short *ptr = (unsigned short *)buffer;

    // poll
    for (int i = 0; i < count; i++) {
        while (!(port_io_input_byte(0x1F7) & 0x08)) {
        }
        for (int i = 0; i < DISK_SECTOR_SIZE / 2; i++) {
            *ptr++ = port_io_input_word(0x1F0);
        }
    }

    return 0;
}