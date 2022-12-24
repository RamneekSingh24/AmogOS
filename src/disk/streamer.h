#ifndef DISK_STREAMER_H
#define DISK_STREAMER_H

#include "disk.h"
#include <stddef.h>

struct disk_stream {
    struct disk *disk;
    size_t byte_offset;
};

struct disk_stream *disk_stream_new(int disk_id);
int disk_stream_seek(struct disk_stream *stream, size_t pos);
int disk_stream_read(struct disk_stream *stream, void *out_buf, size_t size);
void disk_stream_close(struct disk_stream *stream);

void disk_streamer_test();


#endif