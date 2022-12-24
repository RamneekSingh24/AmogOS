#ifndef DISK_H
#define DISK_H

// ATA Disk driver interface for the kernel

typedef unsigned int DISK_TYPE;

#define DISK_TYPE_REAL 0    // REAL PHYSICAL DISK
#define DISK_TYPE_VIRTUAL 1 // VIRTUAL DISK (VFS)

struct disk {
    DISK_TYPE type;
    int sector_size;
};

void disk_init();
struct disk *get_disk(int index);
int disk_read_sectors(struct disk *idisk, int start_lba, int count,
                      void *buffer);

#endif