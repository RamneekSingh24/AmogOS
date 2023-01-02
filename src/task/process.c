#include "process.h"
#include "config.h"
#include "console/console.h"
#include "fs/file.h"
#include "kernel.h"
#include "lib/string/string.h"
#include "loader/elfloader.h"
#include "macros.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "task/task.h"

struct process *current_proc = 0;

struct process procs[MAX_PROCS];

static int process_load_binary(const char *filename, struct process *proc) {
    int res = 0;
    int fd = kfopen(filename, "r");
    if (fd <= 0) {
        res = -STATUS_IO_ERROR;
        goto out;
    }
    struct file_stat stat;
    res = kfstat(fd, &stat);
    if (res != STATUS_OK) {
        goto out;
    }

    uint32_t fsize = stat.file_size;
    void *program_data_ptr = kzalloc(fsize);

    if (!program_data_ptr) {
        res = -STATUS_NOT_ENOUGH_MEM;
        kfclose(fd);
        goto out;
    }

    if (kfread(program_data_ptr, fsize, 1, fd) != 1) {
        res = -STATUS_IO_ERROR;
        kfree(program_data_ptr);
        kfclose(fd);
        goto out;
    }

    proc->file_type = PROC_FILE_TYPE_BINARY;
    proc->code_data_paddr = program_data_ptr;
    proc->size = fsize;

    kfclose(fd);

out:
    return res;
}

static int process_load_elf(const char *filename, struct process *proc) {
    int res = 0;
    struct elf_file *elf_file = 0;
    res = elf_load(filename, &elf_file);
    if (res != STATUS_OK) {
        return res;
    }

    proc->file_type = PROC_FILE_TYPE_ELF;
    proc->elf_file = elf_file;

    return STATUS_OK;
}

static int process_load_data(const char *filename, struct process *proc) {
    int res = process_load_elf(filename, proc);
    if (res == STATUS_INVALID_EXEC_FORMAT) {
        println("Could not load ELF file, trying binary format");
        res = process_load_binary(filename, proc);
    }
    return res;
}

void procs_init() {
    memset(procs, 0, sizeof(procs));
    for (int i = 0; i < MAX_PROCS; i++) {
        procs[i].status = PROC_UNUSED;
    }
}

static void process_init(struct process *process) {
    memset(process, 0, sizeof(struct process));
}

struct process *current_process() { return current_proc; }

int get_free_slot() {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].status == PROC_UNUSED) {
            return i;
        }
    }
    return -1;
}

int proc_free(struct process *proc) {
    if (!proc) {
        return 0;
    }
    if (proc->task) {
        task_free(proc->task);
    }
    // TODO
    return 0;
}

static int process_map_binary(struct process *proc) {
    int res = STATUS_OK;
    struct page_table_32b *pt = &proc->task->page_table;
    void *data_start = (void *)DEFAULT_USER_PROG_ENTRY;
    void *data_end =
        paging_up_align_addr((void *)(DEFAULT_USER_PROG_ENTRY + proc->size));
    res = paging_map_memory_region(
        pt, data_start, proc->code_data_paddr, data_end,
        PAGE_PRESENT | PAGE_WRITE_ALLOW | PAGE_USER_ACCESS_ALLOW);

    if (res != STATUS_OK) {
        return res;
    }

    void *stack_start = (void *)DEFAULT_USER_STACK_START;
    void *stack_end = (void *)DEFAULT_USER_STACK_END;

    // NOTE : stack_start > stack_end
    res = paging_map_memory_region(
        pt, stack_end, proc->stack_paddr, stack_start,
        PAGE_PRESENT | PAGE_WRITE_ALLOW | PAGE_USER_ACCESS_ALLOW);

    return res;
}

// Memory Leak: Does not free any mapped segments in case of error in the middle
static int process_map_elf(struct process *proc) {

    int res = STATUS_OK;

    struct page_table_32b *pt = &proc->task->page_table;

    struct elf_file *elf_file = proc->elf_file;
    struct elf_header *header = elf_header(elf_file);
    struct elf32_phdr *phdrs = elf_pheader(header);
    int ph_count = header->e_phnum;

    for (int i = 0; i < ph_count; i++) {
        struct elf32_phdr *ph = &phdrs[i];

        void *pa_start =
            paging_down_align_addr(elf_phdr_phys_address(elf_file, ph));

        void *va_start = paging_down_align_addr((void *)ph->p_vaddr);
        void *va_end =
            paging_up_align_addr((void *)(ph->p_vaddr + ph->p_memsz));

        int flags = PAGE_PRESENT | PAGE_USER_ACCESS_ALLOW;
        if (ph->p_flags & PF_W) {
            flags |= PAGE_WRITE_ALLOW;
        }

        if (va_end <= va_start) {
            println("[Warning] process_map_elf: empty segment");
            continue;
        }

        res = paging_map_memory_region(pt, va_start, pa_start, va_end, flags);
        if (res != STATUS_OK) {
            return res;
        }
    }

    // map the stack

    void *stack_start = (void *)DEFAULT_USER_STACK_START;
    void *stack_end = (void *)DEFAULT_USER_STACK_END;

    // NOTE : stack_start > stack_end
    res = paging_map_memory_region(
        pt, stack_end, proc->stack_paddr, stack_start,
        PAGE_PRESENT | PAGE_WRITE_ALLOW | PAGE_USER_ACCESS_ALLOW);

    return res;
}

static int process_map_memory(struct process *proc) {
    int res = 0;
    switch (proc->file_type) {
    case PROC_FILE_TYPE_BINARY:
        res = process_map_binary(proc);
        break;
    case PROC_FILE_TYPE_ELF:
        res = process_map_elf(proc);
        break;
    default:
        panic("process_map_memory: can't map invalid exec file type");
        res = -STATUS_INVALID_ARG;
        break;
    }
    return res;
}

struct process *process_current() { return current_proc; }

void set_current_process(struct process *proc) {
    if (current_proc && current_proc->status == PROC_RUNNING &&
        current_proc != proc) {
        current_proc->status = PROC_READY;
    }
    current_proc = proc;
    current_proc->status = PROC_RUNNING;
}

int process_new(const char *filename, struct process **process_out) {
    int res = 0;
    struct task *task = 0;
    void *stack_ptr = 0;

    int pid = get_free_slot();
    if (pid < 0) {
        return -STATUS_OUT_OF_PROCS;
    }

    struct process *proc = &procs[pid];
    process_init(proc);

    res = process_load_data(filename, proc);
    if (res < 0) {
        goto out;
    }

    stack_ptr = kzalloc(DEFAULT_USER_STACK_SIZE);
    if (!stack_ptr) {
        res = -STATUS_NOT_ENOUGH_MEM;
        goto out;
    }

    proc->pid = pid;
    strncpy(proc->program_file, filename, sizeof(proc->program_file));
    proc->stack_paddr = stack_ptr;

    // create task
    task = task_new(proc);
    if (ERROR_I(task) == 0) {
        res = ERROR_I(task);
        goto out;
    }
    proc->task = task;
    res = process_map_memory(proc);
    if (res != STATUS_OK) {
        goto out;
    }
    proc->keyboard.head = 0;
    proc->keyboard.tail = 0;
    memset(proc->keyboard.buf, 0, sizeof(proc->keyboard.buf));
    *process_out = proc;

out:
    if (ISERR(res)) {
        proc_free(proc);
    }
    return res;
}