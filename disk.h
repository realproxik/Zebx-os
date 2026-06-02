#ifndef DISK_H
#define DISK_H

#include "types.h"

void disk_init(void);
int disk_read_sector(uint32_t lba, uint8_t *buffer);
int disk_write_sector(uint32_t lba, uint8_t *buffer);
int disk_read(uint32_t lba, uint32_t count, uint8_t *buffer);
int disk_write(uint32_t lba, uint32_t count, uint8_t *buffer);

#endif

#include "types.h"
#include "vga.h"
#include "memory.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA 0
#define ATA_REG_ERROR 1
#define ATA_REG_SECTOR_COUNT 2
#define ATA_REG_SECTOR_NUM 3
#define ATA_REG_CYL_LOW 4
#define ATA_REG_CYL_HIGH 5
#define ATA_REG_HEAD 6
#define ATA_REG_STATUS 7
#define ATA_REG_COMMAND 7

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_IDENTIFY_PACKET 0xA1

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

typedef struct {
    uint16_t flags;
    uint16_t unused1[9];
    char serial[20];
    uint16_t unused2[3];
    char firmware[8];
    char model[40];
    uint16_t sectors_per_int;
    uint16_t unused3;
    uint16_t capabilities;
    uint16_t unused4;
    uint32_t lba_capacity;
    uint16_t unused5[2];
    uint16_t dma_modes;
    uint16_t pio_modes;
    uint16_t min_dma_cycle;
    uint16_t rec_dma_cycle;
    uint16_t min_pio_cycle;
    uint16_t min_pio_iordy;
} __attribute__((packed)) ata_identify_t;

static uint8_t sector_buffer[512];

static int ata_wait(uint16_t base, int advanced) {
    uint8_t status = 0;
    uint32_t timeout = 1000000;

    while (timeout--) {
        status = inb(base + ATA_REG_STATUS);
        
        if (advanced) {
            if ((status & (ATA_SR_BSY | ATA_SR_ERR)) == 0) {
                break;
            }
            if (status & ATA_SR_ERR) {
                return 0;
            }
        } else {
            if (!(status & ATA_SR_BSY)) {
                break;
            }
        }
    }

    return (timeout > 0) ? 1 : 0;
}

int ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buffer) {
    uint16_t base = ATA_PRIMARY_IO;

    if (!ata_wait(base, 0)) {
        return 0;
    }

    outb(0xE0 | ((lba >> 24) & 0x0F), base + ATA_REG_HEAD);
    ata_wait(base, 0);

    outb(count, base + ATA_REG_SECTOR_COUNT);
    outb((uint8_t)lba, base + ATA_REG_SECTOR_NUM);
    outb((uint8_t)(lba >> 8), base + ATA_REG_CYL_LOW);
    outb((uint8_t)(lba >> 16), base + ATA_REG_CYL_HIGH);

    outb(ATA_CMD_READ_PIO, base + ATA_REG_COMMAND);

    for (int i = 0; i < count; i++) {
        if (!ata_wait(base, 1)) {
            return 0;
        }

        for (int j = 0; j < 256; j++) {
            uint16_t data = inw(base + ATA_REG_DATA);
            buffer[i * 512 + j * 2] = (uint8_t)data;
            buffer[i * 512 + j * 2 + 1] = (uint8_t)(data >> 8);
        }
    }

    return 1;
}

int ata_write_sectors(uint32_t lba, uint8_t count, uint8_t *buffer) {
    uint16_t base = ATA_PRIMARY_IO;

    if (!ata_wait(base, 0)) {
        return 0;
    }

    outb(0xE0 | ((lba >> 24) & 0x0F), base + ATA_REG_HEAD);
    ata_wait(base, 0);

    outb(count, base + ATA_REG_SECTOR_COUNT);
    outb((uint8_t)lba, base + ATA_REG_SECTOR_NUM);
    outb((uint8_t)(lba >> 8), base + ATA_REG_CYL_LOW);
    outb((uint8_t)(lba >> 16), base + ATA_REG_CYL_HIGH);

    outb(ATA_CMD_WRITE_PIO, base + ATA_REG_COMMAND);

    for (int i = 0; i < count; i++) {
        if (!ata_wait(base, 0)) {
            return 0;
        }

        for (int j = 0; j < 256; j++) {
            uint16_t data = buffer[i * 512 + j * 2] | (buffer[i * 512 + j * 2 + 1] << 8);
            outw(data, base + ATA_REG_DATA);
        }
    }

    return 1;
}

int ata_identify(void) {
    uint16_t base = ATA_PRIMARY_IO;

    outb(0xA0, base + ATA_REG_HEAD);
    ata_wait(base, 0);

    outb(ATA_CMD_IDENTIFY, base + ATA_REG_COMMAND);

    if (!ata_wait(base, 1)) {
        return 0;
    }

    ata_identify_t *id = (ata_identify_t *)sector_buffer;
    
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(base + ATA_REG_DATA);
        ((uint8_t *)id)[i * 2] = (uint8_t)data;
        ((uint8_t *)id)[i * 2 + 1] = (uint8_t)(data >> 8);
    }

    return 1;
}

void disk_init(void) {
    if (!ata_identify()) {
        vga_puts("[!] No ATA drive found\\n");
        return;
    }

    vga_puts("[+] ATA drive detected\\n");
}

int disk_read_sector(uint32_t lba, uint8_t *buffer) {
    return ata_read_sectors(lba, 1, buffer);
}

int disk_write_sector(uint32_t lba, uint8_t *buffer) {
    return ata_write_sectors(lba, 1, buffer);
}

int disk_read(uint32_t lba, uint32_t count, uint8_t *buffer) {
    uint32_t remaining = count;
    uint32_t offset = 0;

    while (remaining > 0) {
        uint8_t to_read = (remaining > 255) ? 255 : remaining;
        
        if (!ata_read_sectors(lba, to_read, buffer + offset)) {
            return 0;
        }

        lba += to_read;
        offset += to_read * 512;
        remaining -= to_read;
    }

    return 1;
}

int disk_write(uint32_t lba, uint32_t count, uint8_t *buffer) {
    uint32_t remaining = count;
    uint32_t offset = 0;

    while (remaining > 0) {
        uint8_t to_write = (remaining > 255) ? 255 : remaining;
        
        if (!ata_write_sectors(lba, to_write, buffer + offset)) {
            return 0;
        }

        lba += to_write;
        offset += to_write * 512;
        remaining -= to_write;
    }

    return 1;
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outb(uint8_t val, uint16_t port) {
    asm volatile("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline void outw(uint16_t val, uint16_t port) {
    asm volatile("outw %0, %1" : : "a"(val), "dN"(port));
}