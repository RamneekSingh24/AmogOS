#ifndef FILE_H
#define FILE_H

#include "utils.h"
#include <stdint.h>

typedef unsigned int FILE_SEEK_MODE;

typedef unsigned int FILE_MODE;

typedef unsigned int FILE_STAT_FLAGS;

enum { FILE_SEEK_SET, FILE_SEEK_CUR, FILE_SEEK_END };

enum { FILE_READ, FILE_WRITE, FILE_APPEND, FILE_MODE_INVALID };

enum {
    FILE_STAT_READ_ONLY,
    FILE_STAT_IS_DIR,
    FILE_STAT_IS_FILE,
    FILE_STAT_IS_LINK,
    FILE_STAT_IS_HIDDEN,
    FILE_STAT_IS_SYSTEM,
    FILE_STAT_IS_ARCHIVE,
    FILE_STAT_IS_DEVICE,
};

struct disk;

struct file_stat {
    FILE_STAT_FLAGS flags;
    uint32_t file_size;
};

typedef void *(*FS_OPEN_FUNCTION)(struct disk *disk, struct path_part *path,
                                  FILE_MODE mode);
typedef int (*FS_RESOLVE_FUNCTION)(
    struct disk *disk); // returns 0 if the disk is formmated for the fs.

typedef int (*FS_READ_FUNCTION)(struct disk *disk, void *private_data,
                                uint32_t size, uint32_t nmembs, char *out);

typedef int (*FS_SEEK_FUNCTION)(void *private, uint32_t,
                                FILE_SEEK_MODE seek_mode);

typedef int (*FS_STAT_FUNCTION)(struct disk *disk, void *private,
                                struct file_stat *stat);

typedef int (*FS_CLOSE_FUNCTION)(void *private);

struct file_system {
    char name[20];
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_RESOLVE_FUNCTION resolve;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;
};

struct file_descriptor {
    struct file_system *fs;
    void *private_data; // private data for the file: file inode, loc etc.
    int index;
    struct disk *disk;
};

void fs_init();
int kfopen(const char *filename, const char *mode_str);
int kfread(void *ptr, uint32_t size, uint32_t nmembs, int fd);
int kfseek(int fd, int offset, FILE_SEEK_MODE whence);
int kfstat(int fd, struct file_stat *stat);
int kfclose(int fd);

void fs_insert_filesystem(struct file_system *fs);
struct file_system *fs_resolve(struct disk *disk);

void fs_test();

#endif
