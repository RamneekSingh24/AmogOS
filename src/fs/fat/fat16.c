#include "fat16.h"
#include "console/console.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "lib/string/string.h"
#include "macros.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include <stdint.h>

#define FAT16_SIGNATURE 0x29
#define FAT16_ENTRY_SIZE 0x02
#define FAT16_BAD_SEC 0xFF7
#define FAT16_UNUSED 0x00

typedef unsigned int FAT16_ITEM_TYPE;
// --start -- for internal use only
#define FAT16_ITEM_TYPE_DIR 0
#define FAT16_ITEM_TYPE_FILE 1
// --end   -- for internal use only

// FAT dir entry attribs

#define FAT16_ATTR_READ_ONLY 0x01
#define FAT16_ATTR_HIDDEN 0x02
#define FAT16_ATTR_SYSTEM 0x04
#define FAT16_ATTR_VOLUME_LABEL 0x08
#define FAT16_ATTR_SUB_DIRECTORY 0x10
#define FAT16_ATTR_ARCHIVED 0x20
#define FAT16_FILE_DEVICE 0x40
#define FAT16_FILE_RESERVED 0x80

#define FAT16_MAX_FILE_NAME 8
#define FAT16_MAX_EXT_NAME 3

struct fat16_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];

} __attribute__((packed));

struct fat16_header {
    uint8_t jmp[3]; // short jmp, nop
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));

struct fat16_h {
    struct fat16_header primary_header;
    union fat_h_ext {
        struct fat16_header_extended extended_header;
    } shared;
} __attribute((packed));

struct fat16_dir_entry {
    uint8_t filename[FAT16_MAX_FILE_NAME];
    uint8_t ext[FAT16_MAX_EXT_NAME];
    uint8_t attribs;
    uint8_t _reserved;
    uint8_t creation_time_tenth_of_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t file_size;
} __attribute((packed));

// --start -- internal use only

struct fat16_dir {
    struct fat16_dir_entry *dir_entries;
    int total_entries;
    int dir_start_sector_pos;
    int dir_end_sector_pos;
};

struct fat16_item {
    union fat16 {
        struct fat16_dir_entry *dir_entry;
        struct fat16_dir *dir;
    } item;
    FAT16_ITEM_TYPE item_type;
};

struct fat16_file_descriptor {
    struct fat16_item *item;
    uint32_t pos;
};

struct fat16_private {
    struct fat16_h header;
    struct fat16_dir root_dir;

    // streamer for reading / writing to clusters
    struct disk_stream *cluster_stream;
    // streamer for reading / writing to file allocation table
    struct disk_stream *fat16_read_stream;
    // for reading / writing a directory
    struct disk_stream *dir_stream;
};

// --end  -- internal use only

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode);
int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode);
int fat16_read(struct disk *disk, void *descriptor, uint32_t size,
               uint32_t nmembs, char *out_ptr);
int fat16_stat(struct disk *disk, void *private, struct file_stat *stat);
int fat16_close(void *private);

struct file_system fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close,
};

struct file_system *fat16_init() {
    strncpy(fat16_fs.name, "FAT16", 10);
    return &fat16_fs;
}

static void fat16_init_private(struct disk *disk,
                               struct fat16_private *fat_private) {
    memset(fat_private, 0, sizeof(struct fat16_private));

    fat_private->cluster_stream = disk_stream_new(disk->id);
    fat_private->fat16_read_stream = disk_stream_new(disk->id);
    fat_private->dir_stream = disk_stream_new(disk->id);
}

static int fat16_get_total_items_in_dir(struct disk *disk,
                                        uint32_t dir_start_sector) {
    struct fat16_dir_entry entry;
    int res = 0;
    int cnt = 0;
    struct fat16_private *fat_private = disk->fs_private;
    struct disk_stream *stream = fat_private->dir_stream;

    res = disk_stream_seek(stream, dir_start_sector * disk->sector_size);
    if (res != STATUS_OK) {
        goto out;
    }

    while (1) {
        res = disk_stream_read(stream, &entry, sizeof(entry));
        if (res != STATUS_OK) {
            goto out;
        }
        if (entry.filename[0] == 0x00) {
            // null entry represents end of entries
            break;
        }

        if (entry.filename[0] == 0xE5) {
            // unused entry
            continue;
        }

        cnt++;
    }

    res = cnt;
out:
    return res;
}

static int fat16_load_root_dir(struct disk *disk,
                               struct fat16_private *fat_private) {
    int res = 0;
    struct fat16_header *primary_header = &fat_private->header.primary_header;
    int root_dir_start_sec =
        primary_header->fat_copies * primary_header->sectors_per_fat +
        primary_header->reserved_sectors;

    int root_dir_entries = primary_header->root_dir_entries;
    int root_dir_size = (root_dir_entries * sizeof(struct fat16_dir_entry));
    int total_root_sectors =
        (root_dir_size + disk->sector_size - 1) / disk->sector_size;

    int total_items = fat16_get_total_items_in_dir(disk, root_dir_start_sec);

    if (total_items < 0) {
        res = total_items;
        goto out;
    }

    struct fat16_dir_entry *dir_entries = kzalloc(root_dir_size);
    if (!dir_entries) {
        res = -STATUS_NOT_ENOUGH_MEM;
        goto out;
    }

    struct disk_stream *stream = fat_private->dir_stream;
    res = disk_stream_seek(stream, root_dir_start_sec * disk->sector_size);
    if (res != STATUS_OK) {
        kfree(dir_entries);
        goto out;
    }

    res = disk_stream_read(stream, dir_entries, root_dir_size);
    if (res != STATUS_OK) {
        kfree(dir_entries);
        goto out;
    }

    fat_private->root_dir.dir_entries = dir_entries;
    fat_private->root_dir.total_entries = total_items;
    fat_private->root_dir.dir_start_sector_pos = root_dir_start_sec;
    fat_private->root_dir.dir_end_sector_pos =
        root_dir_start_sec + total_root_sectors;

out:
    return res;
}

int fat16_resolve(struct disk *disk) {
    int res = 0;
    struct fat16_private *fat_private = kzalloc(sizeof(struct fat16_private));
    fat16_init_private(disk, fat_private);
    disk->fs_private = fat_private;
    disk->fs = &fat16_fs;

    struct disk_stream *stream = disk_stream_new(disk->id);
    if (!stream) {
        res = -STATUS_NOT_ENOUGH_MEM;
        goto out;
    }

    res = disk_stream_read(stream, &fat_private->header,
                           sizeof(fat_private->header));
    if (res != STATUS_OK) {
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature !=
        FAT16_SIGNATURE) {
        res = 1; // - for error, 0 for true, > 0 for false
        goto out;
    }

    if (fat16_load_root_dir(disk, fat_private) != STATUS_OK) {
        res = -STATUS_IO_ERROR;
        println("FAT16: failed to load root dir");
        goto out;
    }

    res = 0;

out:
    if (stream) {
        disk_stream_close(stream);
    }
    if (res < 0) {
        kfree(fat_private);
        disk->fs_private = 0;
        disk->fs = 0;
    }
    if (res == 0) {
        println("FAT16: resolved");
    }
    return res;
}

static void fat16_filename_to_proper_str(char **out, const char *in,
                                         int max_len) {
    int i = 0;
    char *out_str = *out;
    while (in[i] != ' ' && in[i] != '\0' && i < max_len) {
        out_str[i] = in[i];
        i++;
    }
    out_str[i] = '\0';
    *out += i;
}

static void fat16_get_full_relative_filename(struct fat16_dir_entry *entry,
                                             char *out, int max_len) {
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_filename_to_proper_str(&out_tmp, (const char *)entry->filename,
                                 FAT16_MAX_FILE_NAME);
    if (entry->ext[0] != ' ' && entry->ext[0] != '\0') {
        *out_tmp = '.';
        out_tmp++;
        fat16_filename_to_proper_str(&out_tmp, (const char *)entry->ext,
                                     FAT16_MAX_EXT_NAME);
    }
}

static int fat16_cluster_to_sector(struct fat16_private *private,
                                   uint32_t cluster) {
    return private->root_dir.dir_end_sector_pos +
           (cluster - 2) * private->header.primary_header.sectors_per_cluster;
}

static int fat16_get_fat_entry(struct disk *disk, int cluster_num) {
    int res = -1;
    struct fat16_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat16_read_stream;
    if (!stream) {
        goto out;
    }
    uint32_t fat_table_byte_pos =
        private->header.primary_header.reserved_sectors * disk->sector_size;
    res = disk_stream_seek(stream,
                           fat_table_byte_pos + cluster_num * FAT16_ENTRY_SIZE);
    if (res < 0) {
        goto out;
    }
    uint32_t result = 0;
    res = disk_stream_read(stream, &result, sizeof(result));
    if (res < 0) {
        goto out;
    }
    res = result;
out:
    return res;
}

static int fat16_get_cluster_at_byte_offset_from_start(struct disk *disk,
                                                       int starting_cluster,
                                                       int offset) {
    int res = 0;
    struct fat16_private *private = disk->fs_private;
    int bytes_per_cluster =
        private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int num_clusters = (offset + 1 + bytes_per_cluster - 1) / bytes_per_cluster;
    int last_cluster = starting_cluster + num_clusters - 1;
    int curr_cluster = starting_cluster;

    for (int next_cluster_i = starting_cluster + 1;
         next_cluster_i <= last_cluster; next_cluster_i++) {
        int entry = fat16_get_fat_entry(disk, curr_cluster);
        if (entry == 0xFF8 || entry == 0xFFF || entry < 0) {
            // no more clusters according to the table
            res = -STATUS_IO_ERROR;
            goto out;
        }
        if (entry == FAT16_BAD_SEC) {
            res = -STATUS_IO_ERROR;
            goto out;
        }
        if (entry == 0xFF0 || entry == 0xFF6) {
            // reserved
            res = -STATUS_IO_ERROR;
            goto out;
        }
        if (entry == 0x00) {
            // corrupted
            res = -STATUS_IO_ERROR;
            goto out;
        }
        curr_cluster = entry;
    }
    res = curr_cluster;
out:
    return res;
}

static int fat16_read_file_data_from_cluster(struct disk *disk,
                                             int starting_cluster, int offset,
                                             int total_bytes, void *out) {
    struct fat16_private *fs_private = disk->fs_private;

    struct disk_stream *stream = fs_private->cluster_stream;

    int res = 0;

    int bytes_per_cluster =
        fs_private->header.primary_header.sectors_per_cluster *
        disk->sector_size;

    while (total_bytes > 0) {
        int curr_cluster = fat16_get_cluster_at_byte_offset_from_start(
            disk, starting_cluster, offset);

        int offset_from_curr_cluster = offset % bytes_per_cluster;

        int curr_sector = fat16_cluster_to_sector(fs_private, curr_cluster);

        int curr_byte_pos =
            (curr_sector * disk->sector_size) + offset_from_curr_cluster;

        int total_to_read_bytes =
            total_bytes > bytes_per_cluster ? bytes_per_cluster : total_bytes;

        res = disk_stream_seek(stream, curr_byte_pos);
        if (res != STATUS_OK) {
            goto out;
        }
        res = disk_stream_read(stream, out, total_to_read_bytes);
        if (res != STATUS_OK) {
            goto out;
        }

        out += total_to_read_bytes;
        offset += total_to_read_bytes;
        total_bytes -= total_to_read_bytes;
    }

out:
    return res;
}

static uint32_t fat16_get_first_cluster(struct fat16_dir_entry *entry) {
    if (entry->high_16_bits_first_cluster != 0) {
        print("Warning non zero high 16 cluster bits in fat16..");
    }
    return (entry->high_16_bits_first_cluster << 16) |
           entry->low_16_bits_first_cluster;
}

static void fat16_free_dir(struct fat16_dir *dir) {
    if (!dir) {
        return;
    }
    if (dir->dir_entries) {
        kfree(dir->dir_entries);
    }
    kfree(dir);
}

static void fat16_free_item(struct fat16_item *item) {
    if (!item) {
        return;
    }
    if (item->item_type == FAT16_ITEM_TYPE_DIR) {
        fat16_free_dir(item->item.dir);
    } else if (item->item_type == FAT16_ITEM_TYPE_FILE) {
        if (item->item.dir_entry) {
            kfree(item->item.dir_entry);
        }
    }
    kfree(item);
}

static struct fat16_dir *fat16_load_fat_dir(struct disk *disk,
                                            struct fat16_dir_entry *entry) {
    int res = 0;
    struct fat16_private *fat_private = disk->fs_private;
    struct fat16_dir *dir = kzalloc(sizeof(struct fat16_dir));
    if (!dir) {
        res = -STATUS_NOT_ENOUGH_MEM;
        goto out;
    }

    uint32_t cluster = fat16_get_first_cluster(entry);
    uint32_t cluster_start_sector =
        fat16_cluster_to_sector(fat_private, cluster);

    // TODO: FIXME: fat16_get_total_items_in_dir only reads contingously from
    // disk without tranversing the linked list of clusters
    int total_items = fat16_get_total_items_in_dir(disk, cluster_start_sector);
    dir->total_entries = total_items;

    int dir_size = total_items * sizeof(struct fat16_dir_entry);
    dir->dir_entries = kzalloc(dir_size);
    if (!dir->dir_entries) {
        res = -STATUS_NOT_ENOUGH_MEM;
        goto out;
    }

    res = fat16_read_file_data_from_cluster(disk, cluster, 0x00, dir_size,
                                            dir->dir_entries);
    if (res != STATUS_OK) {
        res = -1;
        goto out;
    }

out:
    if (res < 0) {
        if (dir && dir->dir_entries) {
            kfree(dir->dir_entries);
        }
        if (dir) {
            kfree(dir);
        }
        dir = 0;
    }
    return dir;
}

static struct fat16_dir_entry *
fat16_clone_dir_entry(struct fat16_dir_entry *entry) {
    struct fat16_dir_entry *f_item = kzalloc(sizeof(struct fat16_dir_entry));
    if (!f_item) {
        return 0;
    }
    memcpy(f_item, entry, sizeof(struct fat16_dir_entry));
    return f_item;
}

static struct fat16_item *
fat16_new_fat_item_from_dir_entry(struct disk *disk,
                                  struct fat16_dir_entry *entry) {
    struct fat16_item *f_item = kzalloc(sizeof(struct fat16_item));
    if (!f_item) {
        return 0;
    }

    if (entry->attribs & FAT16_ATTR_SUB_DIRECTORY) {
        f_item->item.dir = fat16_load_fat_dir(disk, entry);
        f_item->item_type = FAT16_ITEM_TYPE_DIR;
    } else {
        f_item->item.dir_entry = fat16_clone_dir_entry(entry);
        f_item->item_type = FAT16_ITEM_TYPE_FILE;
    }
    return f_item;
}

static struct fat16_item *
fat16_find_item_in_dir_by_name(struct disk *disk, struct fat16_dir *dir,
                               const char *file_name) {
    struct fat16_item *f_item = 0;
    char tmp_filename[FS_MAX_PATH_LEN];
    for (int i = 0; i < dir->total_entries; i++) {
        fat16_get_full_relative_filename(&dir->dir_entries[i], tmp_filename,
                                         sizeof(tmp_filename));

        if (istrncmp(tmp_filename, file_name, sizeof(tmp_filename)) == 0) {
            f_item =
                fat16_new_fat_item_from_dir_entry(disk, &dir->dir_entries[i]);
            break;
        }
    }
    return f_item;
}

static struct fat16_item *fat16_get_dir_entry(struct disk *disk,
                                              struct path_part *path) {
    struct fat16_private *fat_private = disk->fs_private;
    struct fat16_item *curr_item = fat16_find_item_in_dir_by_name(
        disk, &fat_private->root_dir, path->name);
    if (!curr_item) {
        goto out;
    }
    while (path->next) {
        if (curr_item->item_type != FAT16_ITEM_TYPE_DIR) {
            fat16_free_item(curr_item);
            curr_item = 0;
            break;
        }
        path = path->next;
        if (!path->name || path->name[0] == '\0') {
            // empty namr in dir ?
            // open("0:/a/b/") not allowed
            fat16_free_item(curr_item);
            curr_item = 0;
            break;
        }
        struct fat16_item *next_item = fat16_find_item_in_dir_by_name(
            disk, curr_item->item.dir, path->name);
        fat16_free_item(curr_item);
        curr_item = next_item;
    }

out:
    return curr_item;
}

void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode) {
    if (mode != FILE_READ) {
        return ERROR(-STATUS_INVALID_ARG);
    }

    struct fat16_file_descriptor *fd =
        kzalloc(sizeof(struct fat16_file_descriptor));
    if (!fd) {
        return ERROR(-STATUS_NOT_ENOUGH_MEM);
    }

    fd->pos = 0;

    fd->item = fat16_get_dir_entry(disk, path);
    if (!fd->item) {
        kfree(fd);
        return ERROR(-STATUS_IO_ERROR);
    }

    return fd;
}

int fat16_read(struct disk *disk, void *descriptor, uint32_t size,
               uint32_t nmembs, char *out_ptr) {

    int res = 0;
    struct fat16_file_descriptor *fd = descriptor;
    struct fat16_dir_entry *entry = fd->item->item.dir_entry;
    int offset = fd->pos;

    for (int i = 0; i < nmembs; i++) {
        res = fat16_read_file_data_from_cluster(
            disk, fat16_get_first_cluster(entry), offset, size, out_ptr);
        if (ISERR(res)) {
            goto out;
        }
        out_ptr += size;
        offset += size;
    }
    fd->pos = offset;
    res = nmembs;
out:
    return res;
}

int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode) {
    int res = 0;

    struct fat16_file_descriptor *fd = private;
    struct fat16_item *item = fd->item;

    if (item->item_type != FAT16_ITEM_TYPE_FILE) {
        return -STATUS_INVALID_ARG;
    }

    struct fat16_dir_entry *entry = item->item.dir_entry;

    if (offset >= entry->file_size) {
        return -STATUS_IO_ERROR;
    }

    switch (seek_mode) {

    case FILE_SEEK_SET:
        fd->pos = offset;
        break;

    case FILE_SEEK_END:
        res = -STATUS_NOT_IMPLEMENTED;
        break;

    case FILE_SEEK_CUR:
        if (offset + fd->pos >= entry->file_size) {
            res = -STATUS_INVALID_ARG;
        } else {
            fd->pos += offset;
        }
        break;

    default:
        res = -STATUS_INVALID_ARG;
        break;
    }

    return res;
}

int fat16_stat(struct disk *disk, void *private, struct file_stat *stat) {
    struct fat16_file_descriptor *fd = private;
    struct fat16_item *item = fd->item;
    if (item->item_type != FAT16_ITEM_TYPE_FILE) {
        return -STATUS_INVALID_ARG;
    }
    struct fat16_dir_entry *entry = item->item.dir_entry;
    stat->file_size = entry->file_size;
    stat->flags = 0;
    if (entry->attribs & FAT16_ATTR_READ_ONLY) {
        stat->flags |= FILE_STAT_READ_ONLY;
    }
    return 0;
}

static void fat16_fd_free(struct fat16_file_descriptor *fd) {
    if (fd) {
        fat16_free_item(fd->item);
        kfree(fd);
    }
}

int fat16_close(void *private) {
    struct fat16_file_descriptor *fd = private;
    fat16_fd_free(fd);

    return 0;
}