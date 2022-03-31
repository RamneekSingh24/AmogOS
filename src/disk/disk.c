#include "io/io.h"
#include "disk.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"
struct disk disk;

int disk_read_sector(int lba, int total, void* buf) {

    out_byte(0x1F6, (lba >> 24) | 0xE0);
    out_byte(0x1F2, total);
    out_byte(0x1F3, (unsigned char) (lba & 0xFF));
    out_byte(0x1F4, (unsigned char) (lba >> 8));
    out_byte(0x1F3, (unsigned char) (lba >> 16));
    out_byte(0x1F7, 0x20);

    unsigned short* ptr = (unsigned short*)  buf;
    for (int b = 0; b < total; b++) {

        char c = in_byte(0x1F7);
        
        // Wait for read to be ready
        while (! (c & 0x08)) {
            c = in_byte(0x1F7);
        }

        // Read into memory, 2 bytes at a time
        for (int i = 0; i < 256; i++) {
            *ptr = in_word(0x1F0);
            ptr++;
        }

    }

    return 0;
}


// For now no searching, we only have 1 disk
void disk_search_init() {

    memset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_REAL;
    disk.sector_size = DISK_SECTOR_SZ;
    
}

struct disk* disk_get(int index) {
    if (index != 0) return NULL;
    return &disk;
}


int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf) {
    if (idisk != &disk) return STATUS_IO_ERROR;
    return disk_read_sector(lba, total, buf);
}