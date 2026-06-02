#ifndef STORAGE_H
#define STORAGE_H

#include "types.h"

#define STORAGE_TOTAL_SIZE (3ULL * 1024 * 1024 * 1024)  // 3GB
#define STORAGE_BLOCK_SIZE 4096
#define STORAGE_BLOCKS (STORAGE_TOTAL_SIZE / STORAGE_BLOCK_SIZE)

typedef struct {
    uint64_t total_size;
    uint64_t used_size;
    uint64_t free_size;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t used_blocks;
    uint32_t free_blocks;
    char filesystem[16];
} storage_info_t;

void storage_init(void);
storage_info_t* storage_get_info(void);
int storage_read_block(uint32_t block, uint8_t *buffer);
int storage_write_block(uint32_t block, uint8_t *buffer);
int storage_format(const char *fs_type);
uint64_t storage_get_free_space(void);
uint64_t storage_get_used_space(void);

#endif

#include "types.h"
#include "vga.h"
#include "memory.h"

#define STORAGE_PORT_BASE 0x4000
#define STORAGE_CMD_REG 0x00
#define STORAGE_ADDR_REG 0x04
#define STORAGE_DATA_REG 0x08
#define STORAGE_STATUS_REG 0x0C
#define STORAGE_SIZE_REG 0x10

static storage_info_t storage_info = {
    .total_size = STORAGE_TOTAL_SIZE,
    .used_size = 0,
    .free_size = STORAGE_TOTAL_SIZE,
    .block_size = STORAGE_BLOCK_SIZE,
    .total_blocks = STORAGE_BLOCKS,
    .used_blocks = 0,
    .free_blocks = STORAGE_BLOCKS,
    .filesystem = "ZEBXFS"
};

static uint8_t *storage_cache = NULL;
static uint32_t wear_level_table[STORAGE_BLOCKS];

void storage_init(void) {
    // Allocate 3GB virtual storage (simulated)
    storage_cache = (uint8_t *)kmalloc(16 * 1024 * 1024);  // 16MB cache
    if (!storage_cache) {
        vga_puts("[!] Storage: Failed to allocate cache\\n");
        return;
    }
    
    // Initialize wear leveling
    for (uint32_t i = 0; i < STORAGE_BLOCKS; i++) {
        wear_level_table[i] = 0;
    }
    
    // Detect storage
    outl(0x1, STORAGE_PORT_BASE + STORAGE_CMD_REG);
    uint64_t detected_size = ((uint64_t)inl(STORAGE_PORT_BASE + STORAGE_SIZE_REG)) << 32;
    detected_size |= inl(STORAGE_PORT_BASE + STORAGE_SIZE_REG + 4);
    
    if (detected_size > 0) {
        storage_info.total_size = detected_size;
        storage_info.total_blocks = detected_size / STORAGE_BLOCK_SIZE;
        storage_info.free_blocks = storage_info.total_blocks;
    }
    
    vga_puts("[+] 3GB Storage initialized\\n");
    vga_puts("    Total: ");
    vga_put_int(storage_info.total_size / (1024 * 1024 * 1024));
    vga_puts(" GB\\n");
    vga_puts("    Blocks: ");
    vga_put_int(storage_info.total_blocks);
    vga_puts("\\n");
    vga_puts("    Filesystem: ZEBXFS\\n");
}

storage_info_t* storage_get_info(void) {
    return &storage_info;
}

int storage_read_block(uint32_t block, uint8_t *buffer) {
    if (block >= STORAGE_BLOCKS) {
        return 0;
    }
    
    outl(block, STORAGE_PORT_BASE + STORAGE_ADDR_REG);
    outl(0x2, STORAGE_PORT_BASE + STORAGE_CMD_REG);  // Read command
    
    // Wait for ready
    while (inl(STORAGE_PORT_BASE + STORAGE_STATUS_REG) & 0x1);
    
    // Read data
    for (int i = 0; i < STORAGE_BLOCK_SIZE; i += 4) {
        uint32_t data = inl(STORAGE_PORT_BASE + STORAGE_DATA_REG);
        buffer[i] = data & 0xFF;
        buffer[i + 1] = (data >> 8) & 0xFF;
        buffer[i + 2] = (data >> 16) & 0xFF;
        buffer[i + 3] = (data >> 24) & 0xFF;
    }
    
    return 1;
}

int storage_write_block(uint32_t block, uint8_t *buffer) {
    if (block >= STORAGE_BLOCKS) {
        return 0;
    }
    
    // Wear leveling - find least used block
    uint32_t target_block = block;
    uint32_t min_wear = wear_level_table[block];
    
    for (uint32_t i = 0; i < 10; i++) {  // Check 10 nearby blocks
        uint32_t candidate = (block + i) % STORAGE_BLOCKS;
        if (wear_level_table[candidate] < min_wear) {
            min_wear = wear_level_table[candidate];
            target_block = candidate;
        }
    }
    
    wear_level_table[target_block]++;
    
    outl(target_block, STORAGE_PORT_BASE + STORAGE_ADDR_REG);
    outl(0x3, STORAGE_PORT_BASE + STORAGE_CMD_REG);  // Write command
    
    // Write data
    for (int i = 0; i < STORAGE_BLOCK_SIZE; i += 4) {
        uint32_t data = buffer[i] | (buffer[i + 1] << 8) | 
                       (buffer[i + 2] << 16) | (buffer[i + 3] << 24);
        outl(data, STORAGE_PORT_BASE + STORAGE_DATA_REG);
    }
    
    // Update used space
    if (wear_level_table[target_block] == 1) {
        storage_info.used_blocks++;
        storage_info.free_blocks--;
        storage_info.used_size += STORAGE_BLOCK_SIZE;
        storage_info.free_size -= STORAGE_BLOCK_SIZE;
    }
    
    return 1;
}

int storage_format(const char *fs_type) {
    vga_puts("[*] Formatting storage...\\n");
    
    // Reset wear leveling
    for (uint32_t i = 0; i < STORAGE_BLOCKS; i++) {
        wear_level_table[i] = 0;
    }
    
    storage_info.used_blocks = 0;
    storage_info.free_blocks = storage_info.total_blocks;
    storage_info.used_size = 0;
    storage_info.free_size = storage_info.total_size;
    
    int i = 0;
    while (fs_type[i] && i < 15) {
        storage_info.filesystem[i] = fs_type[i];
        i++;
    }
    storage_info.filesystem[i] = '\\0';
    
    vga_puts("[+] Storage formatted with ");
    vga_puts(fs_type);
    vga_puts("\\n");
    
    return 1;
}

uint64_t storage_get_free_space(void) {
    return storage_info.free_size;
}

uint64_t storage_get_used_space(void) {
    return storage_info.used_size;
}

static inline void outl(uint32_t val, uint16_t port) {
    asm volatile("outl %0, %1" : : "a"(val), "dN"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}