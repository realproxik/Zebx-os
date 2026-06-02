@echo off
setlocal enabledelayedexpansion

REM ZEBX OS Build Script - GCC + LD only (no NASM needed)

set GCC_PATH=C:\MinGW\bin\gcc.exe
set LD_PATH=C:\MinGW\bin\ld.exe
set OBJCOPY_PATH=C:\MinGW\bin\objcopy.exe
set QEMU_PATH=C:\Program Files\qemu\qemu-system-i386.exe

if not exist build mkdir build

echo [*] Compiling kernel_main.c
"%GCC_PATH%" -m32 -ffreestanding -O2 -fno-builtin -c kernel\kernel_main.c -o build\kernel_main.o
if errorlevel 1 (
    echo [!] GCC failed!
    pause
    exit /b 1
)

echo [*] Compiling vga.c
"%GCC_PATH%" -m32 -ffreestanding -O2 -fno-builtin -c kernel\vga.c -o build\vga.o
if errorlevel 1 (
    echo [!] GCC failed!
    pause
    exit /b 1
)

echo [*] Linking kernel
"%LD_PATH%" -m elf_i386 -T kernel\linker.ld -o build\kernel.elf build\kernel_main.o build\vga.o
if errorlevel 1 (
    echo [!] Linker failed!
    pause
    exit /b 1
)

echo [*] Converting to binary
"%OBJCOPY_PATH%" -O binary build\kernel.elf build\kernel.bin
if errorlevel 1 (
    echo [!] Objcopy failed!
    pause
    exit /b 1
)

echo [+] Kernel built successfully: build\kernel.bin
echo.
echo [*] Booting ZEBX OS in QEMU...
"%QEMU_PATH%" -fda build\kernel.bin -m 256 -name "ZEBX OS"
pause
