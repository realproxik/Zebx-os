#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

void memory_init(void);
void *kmalloc(uint32_t size);
void kfree(void *ptr);
void memory_stats(void);
uint32_t phys_alloc(void);
void phys_free(uint32_t addr);
void mmap_page(uint32_t vaddr, uint32_t paddr, int user);
void munmap_page(uint32_t vaddr);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#endif
'''

with open('/mnt/agents/output/zebx_os/kernel/memory.h', 'w') as f:
    f.write(memory_h)

# 9. memory.c - Memory management implementation

memory_c = '''// ZEBX OS - Memory Management System

#include "types.h"
#include "vga.h"

typedef struct {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t : 1;
    uint32_t page_size : 1;
    uint32_t : 4;
    uint32_t address : 20;
} pte_t;

typedef struct {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t : 1;
    uint32_t page_size : 1;
    uint32_t : 4;
    uint32_t address : 20;
} pde_t;

static pde_t page_directory[1024] __attribute__((aligned(4096)));
static pte_t page_tables[1024][1024] __attribute__((aligned(4096)));

typedef struct heap_block {
    struct heap_block *next;
    struct heap_block *prev;
    uint32_t size;
    uint32_t allocated;
} heap_block_t;

static heap_block_t *heap_start = NULL;
static heap_block_t *heap_current = NULL;
static uint32_t heap_used = 0;
static uint32_t heap_max = 0x1000000;

static uint8_t phys_bitmap[0x100000];
static uint32_t phys_pages_used = 0;
static uint32_t phys_pages_free = 0;

void memory_init(void) {
    for (int i = 0; i < 1024; i++) {
        page_directory[i].present = 0;
    }

    for (int dir = 0; dir < 4; dir++) {
        uint32_t *pd_entry = (uint32_t*)&page_directory[dir];
        *pd_entry = (uint32_t)&page_tables[dir*256] | 3;

        for (int tbl = 0; tbl < 1024; tbl++) {
            uint32_t *pt_entry = (uint32_t*)&page_tables[dir*256 + tbl][tbl];
            uint32_t phys_addr = (dir * 256 * 4096) + (tbl * 4096);
            *pt_entry = phys_addr | 3;
        }
    }

    uint32_t cr3_value = (uint32_t)&page_directory;
    asm volatile("mov %0, %%cr3" : : "r"(cr3_value));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));

    heap_start = (heap_block_t *)KERNEL_HEAP;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    heap_start->size = 0x10000;
    heap_start->allocated = 0;

    heap_current = heap_start;
    heap_used = sizeof(heap_block_t);

    for (int i = 0; i < 0x100000; i++) {
        phys_bitmap[i] = 0;
    }

    for (int i = 0; i < 256; i++) {
        phys_bitmap[i] = 0xFF;
    }

    phys_pages_used = 256;
    phys_pages_free = (0x100000 - 256);
}

uint32_t phys_alloc(void) {
    for (int i = 256; i < 0x100000; i++) {
        if (phys_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!(phys_bitmap[i] & (1 << j))) {
                    phys_bitmap[i] |= (1 << j);
                    phys_pages_used++;
                    phys_pages_free--;
                    return (i * 8 + j) * PAGE_SIZE;
                }
            }
        }
    }
    return 0;
}

void phys_free(uint32_t addr) {
    uint32_t page_num = addr / PAGE_SIZE;
    uint32_t byte_idx = page_num / 8;
    uint32_t bit_idx = page_num % 8;
    
    if (byte_idx < 0x100000) {
        phys_bitmap[byte_idx] &= ~(1 << bit_idx);
        phys_pages_used--;
        phys_pages_free++;
    }
}

void *kmalloc(uint32_t size) {
    size = (size + 3) & ~3;

    heap_block_t *block = heap_start;
    while (block) {
        if (!block->allocated && block->size >= size) {
            if (block->size > size + sizeof(heap_block_t)) {
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + sizeof(heap_block_t) + size);
                new_block->size = block->size - size - sizeof(heap_block_t);
                new_block->allocated = 0;
                new_block->next = block->next;
                new_block->prev = block;
                
                if (block->next) {
                    block->next->prev = new_block;
                }
                
                block->next = new_block;
                block->size = size;
            }
            
            block->allocated = 1;
            heap_used += size;
            return (void *)((uint8_t *)block + sizeof(heap_block_t));
        }
        block = block->next;
    }

    if (heap_used + size > heap_max) {
        return NULL;
    }

    heap_block_t *new_block = (heap_block_t *)(KERNEL_HEAP + heap_used);
    new_block->size = size;
    new_block->allocated = 1;
    new_block->next = NULL;
    new_block->prev = heap_current;
    
    if (heap_current) {
        heap_current->next = new_block;
    }
    heap_current = new_block;
    
    heap_used += size + sizeof(heap_block_t);
    
    return (void *)((uint8_t *)new_block + sizeof(heap_block_t));
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    block->allocated = 0;
    
    if (block->next && !block->next->allocated) {
        block->size += block->next->size + sizeof(heap_block_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    if (block->prev && !block->prev->allocated) {
        block->prev->size += block->size + sizeof(heap_block_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
    
    heap_used -= block->size;
}

void memory_stats(void) {
    vga_puts("\\n--- Memory Statistics ---\\n");
    vga_puts("Heap used: ");
    vga_put_int(heap_used);
    vga_puts(" bytes\\n");
    vga_puts("Physical pages used: ");
    vga_put_int(phys_pages_used);
    vga_puts("\\n");
    vga_puts("Physical pages free: ");
    vga_put_int(phys_pages_free);
    vga_puts("\\n");
}

void mmap_page(uint32_t vaddr, uint32_t paddr, int user) {
    uint32_t dir_idx = vaddr >> 22;
    uint32_t tbl_idx = (vaddr >> 12) & 0x3FF;

    if (!page_directory[dir_idx].present) {
        uint32_t phys = phys_alloc();
        if (!phys) return;

        page_directory[dir_idx].address = phys >> 12;
        page_directory[dir_idx].present = 1;
        page_directory[dir_idx].writable = 1;
        page_directory[dir_idx].user = user;
    }

    pte_t *pt = (pte_t *)(page_directory[dir_idx].address << 12);
    pt[tbl_idx].address = paddr >> 12;
    pt[tbl_idx].present = 1;
    pt[tbl_idx].writable = 1;
    pt[tbl_idx].user = user;
}

void munmap_page(uint32_t vaddr) {
    uint32_t dir_idx = vaddr >> 22;
    uint32_t tbl_idx = (vaddr >> 12) & 0x3FF;

    if (!page_directory[dir_idx].present) return;

    pte_t *pt = (pte_t *)(page_directory[dir_idx].address << 12);
    pt[tbl_idx].present = 0;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *a = (const uint8_t *)s1;
    const uint8_t *b = (const uint8_t *)s2;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
    }
    return 0;
}