#include "elfloader.h"
#include "config.h"
#include "fs/file.h"
#include "invariants.h"
#include "kernel.h"
#include "lib/string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "status.h"
#include <stdbool.h>

const char elf_signature[] = {0x7f, 'E', 'L', 'F'};

static bool elf_valid_signature(void *buffer) {
    return memcmp(buffer, (void *)elf_signature, sizeof(elf_signature)) == 0;
}

static bool elf_valid_class(struct elf_header *header) {
    // Our kernel is 32 bit.
    return header->e_ident[EI_CLASS] == ELFCLASSNONE ||
           header->e_ident[EI_CLASS] == ELFCLASS32;
}

static bool elf_valid_encoding(struct elf_header *header) {
    return header->e_ident[EI_DATA] == ELFDATANONE ||
           header->e_ident[EI_DATA] == ELFDATA2LSB;
}

static bool elf_is_executable(struct elf_header *header) {
    assert_all_of_kernel_uses_less_than_128mb_phy_adders();

    // KHEAP_SAFE_BOUNDARY is the highest address that the kernel can use.
    // entry point must be above this address.
    // it should also be above some space which is reserved for the stacks.
    // otherwise the stack and code/data might overlap.
    // so we use DEFAULT_USER_PROG_ENTRY as the lower bound instead of
    // KHEAP_SAFE_BOUNDARY.

    // return header->e_type == ET_EXEC && header->e_entry >=
    // KHEAP_SAFE_BOUNDARY;
    return header->e_type == ET_EXEC &&
           header->e_entry >= DEFAULT_USER_PROG_ENTRY;
}

static bool elf_has_program_header(struct elf_header *header) {
    return header->e_phoff != 0;
}

void *elf_memory(struct elf_file *file) { return file->elf_memory; }

struct elf_header *elf_header(struct elf_file *file) {
    return file->elf_memory;
}

struct elf32_shdr *elf_sheader(struct elf_header *header) {
    return (struct elf32_shdr *)((int)header + header->e_shoff);
}

struct elf32_phdr *elf_pheader(struct elf_header *header) {
    if (header->e_phoff == 0) {
        return 0;
    }

    return (struct elf32_phdr *)((int)header + header->e_phoff);
}

struct elf32_phdr *elf_program_header(struct elf_header *header, int index) {
    return &elf_pheader(header)[index];
}

struct elf32_shdr *elf_section(struct elf_header *header, int index) {
    return &elf_sheader(header)[index];
}

char *elf_str_table(struct elf_header *header) {
    return (char *)header + elf_section(header, header->e_shstrndx)->sh_offset;
}

void *elf_virtual_base(struct elf_file *file) {
    return file->virtual_base_address;
}

void *elf_virtual_end(struct elf_file *file) {
    return file->virtual_end_address;
}

void *elf_phys_base(struct elf_file *file) {
    return file->physical_base_address;
}

void *elf_phys_end(struct elf_file *file) { return file->physical_end_address; }

int elf_validate_loaded(struct elf_header *header) {
    return (elf_valid_signature(header) && elf_valid_class(header) &&
            elf_valid_encoding(header) && elf_has_program_header(header))
               ? STATUS_OK
               : -STATUS_INVALID_EXEC_FORMAT;
}

int elf_process_phdr_pt_load(struct elf_file *elf_file,
                             struct elf32_phdr *phdr) {
    if (elf_file->virtual_base_address >= (void *)phdr->p_vaddr ||
        elf_file->virtual_base_address == 0x00) {
        elf_file->virtual_base_address = (void *)phdr->p_vaddr;
        elf_file->physical_base_address = elf_memory(elf_file) + phdr->p_offset;
    }

    unsigned int end_virtual_address = phdr->p_vaddr + phdr->p_filesz;
    if (elf_file->virtual_end_address <= (void *)(end_virtual_address) ||
        elf_file->virtual_end_address == 0x00) {
        elf_file->virtual_end_address = (void *)end_virtual_address;
        elf_file->physical_end_address =
            elf_memory(elf_file) + phdr->p_offset + phdr->p_filesz;
    }
    return 0;
}

void *elf_phdr_phys_address(struct elf_file *file, struct elf32_phdr *phdr) {
    return elf_memory(file) + phdr->p_offset;
}

int elf_process_pheader(struct elf_file *elf_file, struct elf32_phdr *phdr) {
    int res = 0;
    switch (phdr->p_type) {
    case PT_LOAD:
        res = elf_process_phdr_pt_load(elf_file, phdr);
        break;
    }
    return res;
}
int elf_process_pheaders(struct elf_file *elf_file) {
    int res = 0;
    struct elf_header *header = elf_header(elf_file);
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr *phdr = elf_program_header(header, i);
        res = elf_process_pheader(elf_file, phdr);
        if (res < 0) {
            break;
        }
    }
    return res;
}

// checks validity of the elf file and processes the program headers
int elf_process_loaded(struct elf_file *elf_file) {
    int res = 0;
    struct elf_header *header = elf_header(elf_file);
    res = elf_validate_loaded(header);
    if (res < 0) {
        return res;
    }

    res = elf_process_pheaders(elf_file);
    if (res < 0) {
        return res;
    }

    return res;
}
int elf_load(const char *filename, struct elf_file **file_out) {
    struct elf_file *elf_file = kzalloc(sizeof(struct elf_file));
    if (!elf_file) {
        return -STATUS_NOT_ENOUGH_MEM;
    }
    int fd = 0;
    int res = kfopen(filename, "r");
    // NOTE: 0 is invalid fd
    if (res <= 0) {
        kfree(elf_file);
        res = -STATUS_IO_ERROR;
        return res;
    }

    fd = res;
    struct file_stat stat;
    res = kfstat(fd, &stat);
    if (res < 0) {
        kfclose(fd);
        kfree(elf_file);
        return res;
    }

    elf_file->elf_memory = kzalloc(stat.file_size);
    if (!elf_file->elf_memory) {
        kfclose(fd);
        kfree(elf_file);
        return -STATUS_NOT_ENOUGH_MEM;
    }
    res = kfread(elf_file->elf_memory, stat.file_size, 1, fd);
    if (res < 0) {
        kfclose(fd);
        kfree(elf_file->elf_memory);
        kfree(elf_file);
        return res;
    }

    res = elf_process_loaded(elf_file);
    if (res < 0) {
        kfclose(fd);
        kfree(elf_file->elf_memory);
        kfree(elf_file);
        return res;
    }

    *file_out = elf_file;

    kfclose(fd);
    return res;
}

void elf_close(struct elf_file *file) {
    if (!file)
        return;
    if (file->elf_memory) {
        kfree(file->elf_memory);
    }
    kfree(file);
}