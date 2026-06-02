#include "vga.h"

#define VGA_MEM ((volatile uint16_t*)0xB8000)
#define VGA_W 80
#define VGA_H 25
#define ATTR 0x0F00u

static int cx = 0, cy = 0;

static void vga_newline(void) {
    cx = 0;
    cy++;
    if (cy >= VGA_H) cy = 0;
}

void vga_clear(void) {
    for (int y = 0; y < VGA_H; y++) {
        for (int x = 0; x < VGA_W; x++) {
            VGA_MEM[y * VGA_W + x] = (uint16_t)(' ') | ATTR;
        }
    }
    cx = 0; cy = 0;
}

static void vga_putc(char c) {
    if (c == '\n') { vga_newline(); return; }
    VGA_MEM[cy * VGA_W + cx] = ((uint16_t)c) | ATTR;
    cx++;
    if (cx >= VGA_W) vga_newline();
}

void vga_puts(const char *s) {
    if (!s) return;
    while (*s) vga_putc(*s++);
}

void vga_put_hex(uint32_t x) {
    static const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nyb = (x >> (i * 4)) & 0xF;
        vga_putc(hex[nyb]);
    }
}

