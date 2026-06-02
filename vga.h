#ifndef ZEBX_KERNEL_VGA_H
#define ZEBX_KERNEL_VGA_H

#include <stdint.h>

void vga_clear(void);
void vga_puts(const char *s);
void vga_put_hex(uint32_t x);

#endif

