# Zebx

A minimal x86 operating system built from scratch in C, C++, and x86 assembly for deep systems-level learning and bare-metal programming.

## Features

- **Custom Kernel** — Hand-rolled kernel with bootstrap loader and interrupt handling
- **Memory Management** — Physical and virtual memory allocator with paging support
- **Process Scheduling** — Round-robin scheduler with context switching
- **Hardware Abstraction** — Clean HAL for device drivers and port I/O
- **Modular Architecture** — Clean `.c`/`.h` separation for easy extension

## Architecture

zebx/
├── boot/       — Bootloader and startup assembly
├── kernel/     — Core kernel: scheduler, interrupts, syscalls
├── mm/         — Memory management and paging
├── drivers/    — Keyboard, VGA, timer, disk drivers
├── include/    — Shared headers and data structures



## Build Requirements

- `gcc` cross-compiler (i686-elf)
- `nasm` assembler
- `grub-mkrescue` or `xorriso` for ISO generation
- `qemu` for emulation

## Build

```bash
# Clone repository
git clone https://github.com/realproxik/zebx.git
cd zebx

# Build kernel and ISO
make all

# Clean build artifacts
make clean


## qemu installation

# QEMU emulation
qemu-system-x86_64 -cdrom zebx.iso

# Or with debugging
qemu-system-x86_64 -cdrom zebx.iso -s -S

## debug

# Terminal 1: Start QEMU with GDB server
qemu-system-x86_64 -cdrom zebx.iso -s -S

# Terminal 2: Connect GDB
gdb ./kernel.elf
(gdb) target remote localhost:1234
(gdb) break kmain
(gdb) continue
├── lib/        — Kernel standard library (string, printf, etc.)
└── arch/       — x86-specific code (GDT, IDT, ports)
