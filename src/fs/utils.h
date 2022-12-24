#ifndef FS_UTILS_H
#define FS_UTILS_H

struct path_t {
    int drive_no;
    struct path_part *root;
};

struct path_part {
    char *name;
    struct path_part *next;
};

struct path_t *parse_path(const char *path_str);
void free_path(struct path_t *path);
void test_fs_utils();

#endif