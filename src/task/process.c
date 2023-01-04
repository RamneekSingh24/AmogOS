#include "process.h"
#include "config.h"
#include "console/console.h"
#include "fs/file.h"
#include "invariants.h"
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

void process_set_parent_pid(struct process *proc, int pid) {
    proc->parent_pid = pid;
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

int free_pid_slot(int pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == pid) {
            procs[i].status = PROC_UNUSED;
            process_init(procs + i);
            return STATUS_OK;
        }
    }
    return -STATUS_INVALID_ARG;
}

struct process *get_proc_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == pid) {
            return procs + i;
        }
    }
    return 0;
}

// Try to reap process if it is zombie and parent is waiting on it
// returns 0 if process was reaped, -STATUS_PROC_NOT_ZOMBIE if not
int process_reap(struct process *proc) {
    if (proc->status == PROC_ZOMBIE) {
        task_free(proc->task);
        free_pid_slot(proc->pid);
        return STATUS_OK;
    }
    return -STATUS_PROC_NOT_ZOMBIE;
}

// Tries to reap if waitpid is my child and it is zombie
// returns O if process was reaped, -error otherwise
int process_waitpid(struct process *proc, int waitpid) {
    struct process *waitproc = get_proc_by_pid(waitpid);
    if (!waitproc || waitproc->parent_pid != proc->pid) {
        return -STATUS_INVALID_ARG;
    }
    return process_reap(waitproc);
}

int process_exit(struct process *proc, int status) {
    if (!proc) {
        return 0;
    }
    assert_single_task_per_process();

    // Note: Do not free the task as we may be in the kernel context of the task
    // itself Free the task in reap, where we will be sure that we are not in
    // this task's context if (proc->task) {
    //     task_free(proc->task);
    // }
    proc->task->state = TASK_DEAD;

    // TODO: We should not use these kind of freeing as pages may be swapped out
    // For now though this is fine. Change this later to paging_free_va,
    // when we introduce separate heap for user processes.

    // safe to free all of this because we running in the global kernel stack
    // or kstack of the proc
    if (proc->file_type == PROC_FILE_TYPE_BINARY) {
        kfree(proc->code_data_paddr);
    } else if (proc->file_type == PROC_FILE_TYPE_ELF) {
        kfree(elf_memory(proc->elf_file));
    }

    for (int i = 0; i < PROCESS_VMEM_MAX_BLOCKS; i++) {
        if (proc->vmem_blocks_start[i] != 0) {
            process_free_vmem_block(proc, proc->vmem_blocks_start[i]);
        }
    }

    if (proc->stack_paddr) {
        kfree(proc->stack_paddr);
    }

    if (proc == current_proc) {
        current_proc = 0;
    }

    // waiting for parent to reap
    proc->status = PROC_ZOMBIE;
    proc->exit_status = status;

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

int process_add_vmem_block(struct process *proc, void *va_start, void *va_end) {
    for (int i = 0; i < PROCESS_VMEM_MAX_BLOCKS; i++) {
        if (proc->vmem_blocks_start[i] == 0) {
            proc->vmem_blocks_start[i] = va_start;
            proc->vmem_blocks_end[i] = va_end;
            return i;
        }
    }
    return -STATUS_OUT_OF_VMEM_BLOCKS;
}

int process_get_vmem_block(struct process *proc, void *va_start) {
    for (int i = 0; i < PROCESS_VMEM_MAX_BLOCKS; i++) {
        if (proc->vmem_blocks_start[i] == va_start) {
            return i;
        }
    }
    return -STATUS_INVALID_ARG;
}

int process_free_vmem_block(struct process *proc, void *va_start) {
    int block_id = process_get_vmem_block(proc, va_start);
    if (block_id < 0) {
        return block_id;
    }
    // should loop over all tasks and free and do tlb shootdown
    // but for now we support only 1 task per process
    struct page_table_32b *pt = &proc->task->page_table;
    void *va_end = proc->vmem_blocks_end[block_id];

    // TODO: For now this does not free anything actually,
    // freeing is deferred until shared pages are introduced
    paging_free_va(pt, (uint32_t)va_start, (uint32_t)va_end);

    proc->vmem_blocks_start[block_id] = 0;
    proc->vmem_blocks_end[block_id] = 0;
    return STATUS_OK;
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

        if (ph->p_type != PT_LOAD) {
            continue;
        }

        void *pa_start =
            paging_down_align_addr(elf_phdr_phys_address(elf_file, ph));

        if (ph->p_memsz == 0) {
            println("[Warning] process_map_elf: empty segment");
            continue;
        }

        if (ph->p_filesz != ph->p_memsz) {
            if (ph->p_filesz > ph->p_memsz) {
                panic("process_map_elf: invalid segment, p_filesz > p_memsz");
            }

            pa_start = kzalloc(ph->p_memsz);
            if (!pa_start) {
                return -STATUS_NOT_ENOUGH_MEM;
            }
            // If segement's memory size mem_size is greater than its file size
            // then the "extra" bytes are defined to hold the value 0 and
            // the bytes from the file are mapped to the beginning of the
            // memory segment.
            // Source:
            // https://www.cs.cmu.edu/afs/cs/academic/class/15213-f00/docs/elf.pdf
            // Page 34
            memcpy(pa_start, elf_phdr_phys_address(elf_file, ph), ph->p_filesz);

            // Don't forget to free this memory
            // Done: memory added as a vmem heap block
            process_add_vmem_block(proc, pa_start, pa_start + ph->p_memsz);
        }

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

        // TODO: For now most of the times pa_start is part of the elf_file
        // which we copied into memory Later on we would want to use a separate
        // heap for processes and only map memory from there and not from the
        // elf_file. So than we can manage swapping and freeing out of memory.

        // TODO: memory leak, don't forget to free the elf_file_memory somewhere
        // else when we do that, for now it is freed in process_free

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
    if (proc->status != PROC_CAN_START) {
        panic("set_current_process called, but process is not ready to run");
    }
    current_proc = proc;
}

int process_new(const char *filename, struct process **process_out) {
    int res = 0;
    struct task *task = 0;

    int pid = get_free_slot();
    if (pid < 0) {
        return -STATUS_OUT_OF_PROCS;
    }

    struct process *proc = &procs[pid];
    process_init(proc);

    // allocate stack for main thread of the process
    void *stack_ptr = kzalloc(DEFAULT_USER_STACK_SIZE);
    if (!stack_ptr) {
        res = -STATUS_NOT_ENOUGH_MEM;
        goto out;
    }
    proc->stack_paddr = stack_ptr;

    res = process_load_data(filename, proc);
    if (res < 0) {
        goto out;
    }

    proc->pid = pid;
    strncpy(proc->program_file, filename, sizeof(proc->program_file));

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
    proc->pid = pid;
    proc->status = PROC_CREATING;
    memset(proc->keyboard.buf, 0, sizeof(proc->keyboard.buf));
    *process_out = proc;

out:
    if (ISERR(res)) {
        // TODO: free memory
        // proc_free(proc);
    }
    return res;
}

// args should point to start of the arguments, which are joined by the null
// terminator i.e. args = location("\0".join(arguments)) len should be the
// length of the args string (including all the null terminator) Example:
// argc=3, len = **12**, args = location("abc\0def\0ghi\0") int
// create_proccess(const char* file_path, int argc, int len, char* args);
int process_add_arguments(struct process *proc, int argc, int len, char *args) {
    int res = STATUS_OK;

    // proc->status = PROC_CAN_START;
    // void* stk = proc->stack_paddr + DEFAULT_USER_STACK_SIZE;
    // // push argc and argv
    // stk -= sizeof(int);
    // print_int(*(int*)stk);
    // println("");
    // stk -= sizeof(int);
    // print_int(*(int*)stk);
    // *(int*) stk = argc;
    // proc->task->registers.esp -= (sizeof(int) + sizeof(char**));
    // return 0;

    if (proc->status != PROC_CREATING) {
        panic("process_add_arguments: process is not in creating state");
    }

    // We first push all the arguments to the stack, then we push the pointers

    if (sizeof(char *) * argc + len > DEFAULT_USER_STACK_SIZE) {
        // Too many arguments which we can't fit in the stack
        return -STATUS_NOT_ENOUGH_MEM;
    }

    void *stack = proc->stack_paddr + DEFAULT_USER_STACK_SIZE;
    uint32_t *esp = &proc->task->registers.esp;

    if (argc == 0) {
        goto push_argc_argv;
    }

    // push args in the stack
    stack -= len;
    *esp -= len;
    char *args_buf = (char *)stack;
    char *args_buf_va = (char *)*esp; // virtual addresses
    memcpy(args_buf, args, len);

    // push arg pointers in the stack
    stack -= sizeof(char *) * argc;
    *esp -= sizeof(char *) * argc;
    char **arg_ptrs = (char **)stack;
    char **arg_ptrs_va = (char **)*esp; // virtual addresses

    // set the pointers, be careful to use virtual addresses
    // here argc >= 1
    char *curr_arg = args_buf;
    char *curr_arg_va = args_buf_va;
    arg_ptrs[0] = curr_arg_va;
    for (int i = 1; i < argc; i++) {
        int len = strlen(curr_arg) + 1; // +1 for the null terminator
        curr_arg += len;
        curr_arg_va += len;
        arg_ptrs[i] = curr_arg_va;
    }

push_argc_argv:

    // push argv
    stack -= sizeof(char **);
    *(char ***)stack = arg_ptrs_va;

    // push argc
    stack -= sizeof(int);
    *(int *)stack = argc;

    // set esp to pretend we just pushed argc and argv
    *esp -= (sizeof(int) + sizeof(char **));
    proc->status = PROC_CAN_START;
    return res;
}
