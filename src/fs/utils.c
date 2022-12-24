#include "utils.h"
#include "config.h"
#include "lib/string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "console/console.h"
#include "status.h"

static bool is_path_valid(const char *path) {
    int len = strnlen(path, FS_MAX_PATH_LEN);
    return len >= 3 && is_digit(path[0]) && path[1] == ':' && path[2] == '/';
}

struct path_t *parse_path(const char *path_str) {
    if (!is_path_valid(path_str)) {
        return NULL;
    }
    int drive_no = path_str[0] - '0';
    int len = strnlen(path_str, FS_MAX_PATH_LEN);

    char *path_str_copy = kzalloc(len + 1);
    memcpy(path_str_copy, path_str, len);
    path_str_copy[len] = '\0';

    struct path_t *path = kzalloc(sizeof(struct path_t));

    struct path_part *root = kzalloc(sizeof(struct path_part));

    path->drive_no = drive_no;
    path->root = root;

    root->name = &path_str_copy[3];
    root->next = NULL;

    for (int i = 3; i < len; i++) {
        if (path_str_copy[i] == '/') {
            path_str_copy[i] = '\0';
            struct path_part *part = kzalloc(sizeof(struct path_part));
            part->name = &path_str_copy[i + 1];
            part->next = NULL;
            root->next = part;
            root = part;
        }
    }

    return path;
}

void free_path(struct path_t *path) {
    kfree(path->root->name);
    struct path_part *part = path->root;
    while (part != NULL) {
        struct path_part *next = part->next;
        kfree(part);
        part = next;
    }
    kfree(path);
}


// -----  tests ------------- //


void test_parse_path() {
    print("\n at start: ");
    print_int(kheap_num_free_blocks());
    println("");
    struct path_t *path = parse_path("0:/");
    if (path == NULL) {
        print("path is NULL");
    }
    if (path->drive_no != 0) {
        print("drive_no is not 0");
        return;
    }
    if (path->root->name[0] != '\0') {
        print("root name is not empty");
        return;
    }
    if (path->root->next != NULL) {
        print("root next is not NULL");
        return;
    }
    free_path(path);

    path = parse_path("0:/a");
    if (path == NULL) {
        print("path is NULL");
        return;
    }
    if (path->drive_no != 0) {
        print("drive_no is not 0");
        return;
    }
    if (memcmp(path->root->name, "a", 1) != 0) {
        print("root name is not a");
        return;
    }
    if (path->root->next != NULL) {
        print("root next is not NULL");
        return;
    }
    free_path(path);

    path = parse_path("0:/a/bd/c");
    if (path == NULL) {
        print("path is NULL");
        return;
    }
    if (path->drive_no != 0) {
        print("drive_no is not 0");
        return;
    }
    if (memcmp(path->root->name, "a", 1) != 0) {
        print("root name is not a");
        return;
    }
    if (memcmp(path->root->next->name, "bd", 2) != 0) {
        print("root next name is not bd");
        return;
    }
    if (memcmp(path->root->next->next->name, "c", 1) != 0) {
        print("root next next name is not c");
        return;
    }
    if (path->root->next->next->next != NULL) {
        print("root next next next is not NULL");
        return;
    }
    
    free_path(path);
    print("\n at end: ");
    print_int(kheap_num_free_blocks());
    println("");
    print("test_parse_path passed");
}


void test_fs_utils() {
    test_parse_path();
}