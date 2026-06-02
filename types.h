#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

// Basic types
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef uint32_t size_t;
typedef int32_t ssize_t;
typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

#define NULL ((void *)0)
#define TRUE 1
#define FALSE 0

// Page size
#define PAGE_SIZE 4096
#define PAGE_ALIGN(x) ((x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Memory constants
#define KERNEL_BASE 0x10000
#define KERNEL_HEAP 0x100000
#define USER_BASE 0x400000

// Process constants
#define MAX_PROCESSES 256
#define MAX_OPEN_FILES 16
#define MAX_THREADS 8

// Filesystem constants
#define MAX_FILES 512
#define MAX_FILENAME 255
#define MAX_PATH 512
#define FS_BLOCK_SIZE 4096

// Device constants
#define MAX_DEVICES 32
#define ATA_PRIMARY 0x1F0
#define ATA_SECONDARY 0x170

// Shell constants
#define MAX_COMMAND_LENGTH 512
#define MAX_ARGS 32

// VGA constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Colors
enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15,
};

// IRQ assignments
#define IRQ_TIMER 0
#define IRQ_KEYBOARD 1
#define IRQ_COM2 3
#define IRQ_COM1 4
#define IRQ_LPT2 5
#define IRQ_FLOPPY 6
#define IRQ_LPT1 7
#define IRQ_RTC 8
#define IRQ_MOUSE 12
#define IRQ_IDE1 14
#define IRQ_IDE2 15

// File modes
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_CREAT 0x0040
#define O_TRUNC 0x0200

// File types
#define S_IFREG 0x8000
#define S_IFDIR 0x4000
#define S_IFBLK 0x6000
#define S_IFCHR 0x2000

#endif
