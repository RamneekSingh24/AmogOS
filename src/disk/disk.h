#ifndef DISK_H
#define DISK_H

typedef unsigned int DISK_TYPE;


// Real physical hard disk
#define DISK_TYPE_REAL 0
#define DISK_VIRTIO 1

struct disk
{
    DISK_TYPE type;
    int sector_size;
};



void disk_search_init();
struct disk* disk_get(int index);
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);

#endif