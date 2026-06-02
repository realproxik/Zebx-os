#include <stdint.h>
#include "vga.h"

// Multiboot2 header (GRUB will load us)
__attribute__((section(".multiboot"), used))
static const uint32_t mb2_header[] = {
    0xE85250D6u,         // magic
    0u,                  // architecture (i386)
    24u,                 // header length
    0u - (0xE85250D6u + 0u + 24u), // checksum

    0u, 0u               // end tag (type=0, flags=0, size=8) (padded by linker)
};

void kmain(uint32_t mb_magic, uint32_t mb_info) {
    vga_clear();
    vga_puts("ZEBX OS: booted into kernel\n");
    vga_puts("multiboot magic: "); vga_put_hex(mb_magic); vga_puts("\n");
    vga_puts("multiboot info : "); vga_put_hex(mb_info);  vga_puts("\n");
    vga_puts("\nNext: we will wire your real drivers + shell here.\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}

