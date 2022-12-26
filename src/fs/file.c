#include "file.h"
#include "config.h"
#include "console/console.h"
#include "disk/disk.h"
#include "fat/fat16.h"
#include "fs/file.h"
#include "lib/string/string.h"
#include "macros.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "utils.h"
// Path: src/fs/file.c

struct file_system *filesystems[MAX_FILESYSTEMS];
struct file_descriptor *file_descriptors[MAX_FILE_DESCRIPTORS];

static struct file_system **get_free_filesystem() {
    for (int i = 0; i < MAX_FILESYSTEMS; i++) {
        if (filesystems[i] == 0) {
            return &filesystems[i];
        }
    }
    return 0;
}

void fs_insert_filesystem(struct file_system *fs) {
    struct file_system **free_fs = get_free_filesystem();
    if (free_fs == 0) {
        println("No more free filesystem slots");
        while (1) {
        };
        return;
    }
    *free_fs = fs;
}

// load the fs from the kernel boot image sector
static void kfs_load() { fs_insert_filesystem(fat16_init()); }

void fs_init() {
    for (int i = 0; i < MAX_FILESYSTEMS; i++) {
        filesystems[i] = 0;
    }
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        file_descriptors[i] = 0;
    }
    kfs_load();
}

static int new_file_descriptor(struct file_descriptor **fd_out) {
    int res = -STATUS_NOT_ENOUGH_MEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == 0) {
            file_descriptors[i] = kzalloc(sizeof(struct file_descriptor));
            if (file_descriptors[i] == 0) {
                res = -STATUS_NOT_ENOUGH_MEM;
                break;
            }
            *fd_out = file_descriptors[i];
            file_descriptors[i]->index = i + 1;
            res = 0;
            break;
        }
    }
    return res;
}

static struct file_descriptor *get_file_descriptor(int fd) {
    if (fd <= 0 || fd > MAX_FILE_DESCRIPTORS) {
        return 0;
    }
    return file_descriptors[fd - 1];
}

static void free_file_descriptor(int fd) {
    if (fd <= 0 || fd >= MAX_FILE_DESCRIPTORS) {
        return;
    }
    kfree(file_descriptors[fd - 1]);
    file_descriptors[fd - 1] = 0;
}

struct file_system *fs_resolve(struct disk *disk) {
    for (int i = 0; i < MAX_FILESYSTEMS; i++) {
        if (filesystems[i] != 0) {
            if (filesystems[i]->resolve(disk) == 0) {
                return filesystems[i];
            }
        }
    }
    return 0;
}

static FILE_MODE parse_mode(const char *mode) {
    if (strncmp(mode, "r", 1) == 0) {
        return FILE_READ;
    } else if (strncmp(mode, "w", 1) == 0) {
        return FILE_WRITE;
    } else if (strncmp(mode, "a", 1) == 0) {
        return FILE_APPEND;
    }
    return FILE_MODE_INVALID;
}

int kfopen(const char *filename, const char *mode_str) {
    int res = 0;

    struct path_t *root_path = parse_path(filename);

    if (!root_path) {
        res = -STATUS_BAD_FILE_PATH;
        goto out;
    }

    if (root_path->root->name[0] == '\0') {
        // "0:/"
        res = -STATUS_BAD_FILE_PATH;
        goto out;
    }

    struct disk *disk = get_disk(root_path->drive_no);

    if (!disk) {
        res = -STATUS_BAD_FILE_PATH;
        goto out;
    }

    if (!disk->fs) {
        res = -STATUS_IO_ERROR;
        goto out;
    }

    FILE_MODE mode = parse_mode(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -STATUS_INVALID_ARG;
        goto out;
    }

    void *descriptor_private_data = disk->fs->open(disk, root_path->root, mode);

    if (ISERR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor *fd = 0;
    res = new_file_descriptor(&fd);
    if (res < 0) {
        goto out;
    }

    fd->fs = disk->fs;
    fd->private_data = descriptor_private_data;
    fd->disk = disk;
    res = fd->index;

out:
    if (res < 0) {
        // should only return 0 (NULL) or positive fd.
        res = 0;
    }
    return res;
}

int kfseek(int fd, int offset, FILE_SEEK_MODE whence) {
    int res = 0;
    struct file_descriptor *file_descriptor = get_file_descriptor(fd);

    if (!file_descriptor) {
        res = -STATUS_INVALID_ARG;
        goto out;
    }

    res = file_descriptor->fs->seek(file_descriptor->private_data, offset,
                                    whence);

out:
    return res;
}

int kfread(void *ptr, uint32_t size, uint32_t nmembs, int fd) {
    int res = 0;
    if (size == 0 || nmembs == 0 || fd < 1) {
        res = -STATUS_INVALID_ARG;
        goto out;
    }

    struct file_descriptor *file_descriptor = get_file_descriptor(fd);

    if (!file_descriptor) {
        res = -STATUS_INVALID_ARG;
        goto out;
    }

    res = file_descriptor->fs->read(file_descriptor->disk,
                                    file_descriptor->private_data, size, nmembs,
                                    (char *)ptr);

out:
    return res;
}

int kfstat(int fd, struct file_stat *stat) {
    struct file_descriptor *file_descriptor = get_file_descriptor(fd);
    if (!file_descriptor) {
        return -STATUS_INVALID_ARG;
    }

    return file_descriptor->fs->stat(file_descriptor->disk,
                                     file_descriptor->private_data, stat);
}

int kfclose(int fd) {
    struct file_descriptor *file_descriptor = get_file_descriptor(fd);
    if (!file_descriptor) {
        return -STATUS_INVALID_ARG;
    }
    int res = file_descriptor->fs->close(file_descriptor->private_data);

    free_file_descriptor(fd);

    return res;
}

// ----------- tests ------------- //

void fs_test() {
    int fd = kfopen("0:/hello.txt", "r");
    if (fd) {
        println("hello.txt opened sucessfully");
        char buf[64];
        for (int i = 0; i < 64; i++) {
            buf[i] = 0;
        }
        int res = kfread(buf, 6, 1, fd);
        println(buf);
        print_int(res);
        println("");

        for (int i = 0; i < 64; i++) {
            buf[i] = 0;
        }
        res = kfread(buf, 6, 1, fd);
        println(buf);
        print_int(res);
        println("");

        kfseek(fd, 2, FILE_SEEK_SET);
        for (int i = 0; i < 64; i++) {
            buf[i] = 0;
        }
        res = kfread(buf, 6, 1, fd);
        println(buf);
        print_int(res);
        println("");

        struct file_stat stat;
        kfstat(fd, &stat);
        println("file stats:");
        print_int(stat.file_size);
        println("");
        print_int(stat.flags);
        println("");

        println("closing file..");
        res = kfclose(fd);
        print_int(res);
        println("");

    } else {
        println("failed to open hello.txt");
    }
}