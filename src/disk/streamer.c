#include "streamer.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include "console/console.h"


struct disk_stream *disk_stream_new(int disk_id) {

    struct disk* disk = get_disk(disk_id);
    if (disk == NULL) {
        return NULL;
    }

    struct disk_stream *stream = kzalloc(sizeof(struct disk_stream));
    stream->disk = disk;
    stream->byte_offset = 0;
    return stream;
}

int disk_stream_seek(struct disk_stream *stream, size_t pos) {
    stream->byte_offset = pos;
    return 0;
}

int disk_stream_read(struct disk_stream *stream, void *out_buf, size_t size) {
    struct disk *disk = stream->disk;
    char* buf = kzalloc(disk->sector_size);
    size_t end = stream->byte_offset + size;
    int ret;


    while (stream->byte_offset < end) {
        size_t lba = stream->byte_offset / disk->sector_size;
        size_t lba_offset = stream->byte_offset % disk->sector_size;
        int to_copy = end - stream->byte_offset;
        if (lba_offset + to_copy > disk->sector_size) {
            to_copy = disk->sector_size - lba_offset;
        }
        ret = disk_read_sectors(disk, lba, 1, buf);
        if (ret != 0) {
            goto out;
        }
        memcpy(out_buf, buf + lba_offset, to_copy);
        out_buf += to_copy;
        stream->byte_offset += to_copy;
    }

out:
    kfree(buf);
    return ret;
}


void disk_stream_close(struct disk_stream *stream) {
    kfree(stream);
}


// ------- tests ------------ //

void disk_streamer_test() {
    struct disk_stream *stream = disk_stream_new(0);
    char expected[2048];
    disk_read_sectors(stream->disk, 0, 4, expected);
    char* buf = kzalloc(2048);
    disk_stream_read(stream, buf, 1);
    
    if (buf[0] != expected[0]) {
        println("error: invalid stream read at 0");
        return;
    }
    
    disk_stream_read(stream, buf, 1);

    if (buf[0] != expected[1]) {
        println("error: invalid stream read at 1");
        return;
    }

    disk_stream_read(stream, buf, 10);

    if (memcmp(&buf[0], &expected[2], 10) != 0) {
        println("error: invalid stream read at 2 of 10");
        return;
    }

    disk_stream_read(stream, buf, 1000);

    if (memcmp(&buf[0], &expected[12], 1000) != 0) {
        println("error: invalid stream read at 12 of 1000");
        return;
    }
    
    disk_stream_seek(stream, 505);
    disk_stream_read(stream, buf, 1000);


    if (memcmp(&buf[0], &expected[505], 1000) != 0) {
        println("error: invalid stream read at 505 of 1000");
        return;
    }




    kfree(buf);
    disk_stream_close(stream);
    print("disk stream tests passed");
}