#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

typedef struct process process_t;

void process_init(void);
pid_t process_create(const char *name, void (*entry)(void), process_t *parent);
void process_terminate(pid_t pid, int exit_code);
void process_switch(void);
process_t *process_current(void);
process_t *process_get(pid_t pid);
void process_list(void);

#endif


#include "types.h"
#include "memory.h"
#include "vga.h"

typedef struct {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t eflags;
} __attribute__((packed)) registers_t;

typedef enum {
    PROCESS_CREATED,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_READY,
    PROCESS_TERMINATED,
} process_state_t;

typedef struct process {
    pid_t pid;
    char name[64];
    process_state_t state;
    
    registers_t registers;
    uint32_t kernel_stack;
    uint32_t user_stack;
    
    uint32_t page_directory;
    
    int exit_code;
    struct process *parent;
    struct process *next;
    struct process *prev;
} process_t;

static process_t processes[MAX_PROCESSES];
static int process_count = 0;
static process_t *current_process = NULL;
static process_t *process_queue = NULL;

static pid_t next_pid = 1;

process_t *process_get(pid_t pid) {
    for (int i = 0; i < process_count; i++) {
        if (processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return NULL;
}

pid_t process_create(const char *name, void (*entry)(void), process_t *parent) {
    if (process_count >= MAX_PROCESSES) {
        return -1;
    }

    process_t *proc = &processes[process_count++];
    
    proc->pid = next_pid++;
    
    int i = 0;
    while (name[i] && i < 63) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\\0';
    
    proc->state = PROCESS_CREATED;
    proc->parent = parent;
    proc->exit_code = 0;
    proc->next = NULL;
    proc->prev = NULL;

    proc->kernel_stack = (uint32_t)kmalloc(8192);
    if (!proc->kernel_stack) {
        process_count--;
        return -1;
    }

    proc->user_stack = (uint32_t)kmalloc(8192);
    if (!proc->user_stack) {
        kfree((void *)proc->kernel_stack);
        process_count--;
        return -1;
    }

    memset(&proc->registers, 0, sizeof(registers_t));
    proc->registers.eip = (uint32_t)entry;
    proc->registers.esp = proc->user_stack + 8192;
    proc->registers.ebp = proc->user_stack + 8192;
    proc->registers.eflags = 0x202;

    proc->state = PROCESS_READY;

    if (!process_queue) {
        process_queue = proc;
    } else {
        process_t *p = process_queue;
        while (p->next) {
            p = p->next;
        }
        p->next = proc;
        proc->prev = p;
    }

    return proc->pid;
}

void process_terminate(pid_t pid, int exit_code) {
    process_t *proc = process_get(pid);
    if (!proc) return;

    proc->state = PROCESS_TERMINATED;
    proc->exit_code = exit_code;

    if (proc->prev) {
        proc->prev->next = proc->next;
    } else {
        process_queue = proc->next;
    }

    if (proc->next) {
        proc->next->prev = proc->prev;
    }

    kfree((void *)proc->kernel_stack);
    kfree((void *)proc->user_stack);
}

void process_switch(void) {
    if (!process_queue) {
        return;
    }

    process_t *next = process_queue;
    while (next && next->state != PROCESS_READY) {
        next = next->next;
        if (!next) {
            next = process_queue;
            break;
        }
    }

    if (!next) return;

    if (current_process) {
        current_process->state = PROCESS_READY;
    }

    current_process = next;
    current_process->state = PROCESS_RUNNING;
}

process_t *process_current(void) {
    return current_process;
}

void process_init(void) {
    memset(processes, 0, sizeof(processes));
    process_count = 0;
    current_process = NULL;
    process_queue = NULL;
    next_pid = 1;
}

void process_list(void) {
    vga_puts("\\n--- Running Processes ---\\n");
    vga_puts("PID  Name                State\\n");
    vga_puts("---  ----                -----\\n");

    for (int i = 0; i < process_count; i++) {
        process_t *p = &processes[i];
        
        vga_put_int(p->pid);
        vga_puts("    ");
        
        vga_puts(p->name);
        for (int j = strlen(p->name); j < 20; j++) {
            vga_putchar(' ');
        }
        
        switch (p->state) {
            case PROCESS_RUNNING:
                vga_puts("Running");
                break;
            case PROCESS_READY:
                vga_puts("Ready");
                break;
            case PROCESS_WAITING:
                vga_puts("Waiting");
                break;
            case PROCESS_TERMINATED:
                vga_puts("Terminated");
                break;
            default:
                vga_puts("Unknown");
                break;
        }
        
        vga_puts("\\n");
    }

    vga_puts("\\nTotal: ");
    vga_put_int(process_count);
    vga_puts(" processes\\n");
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

void strncpy(char *dest, const char *src, size_t n) {
    for (size_t i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *dest, const char *src) {
    char *start = dest;
    while ((*dest++ = *src++));
    return start;
}