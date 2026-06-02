@echo off
setlocal enabledelayedexpansion

REM Simple ZEBX OS Boot Script
REM Uses pre-built kernel binary if available

echo.
echo ============================================
echo    ZEBX OS Bootstrap
echo ============================================
echo.

set QEMU_PATH=C:\Program Files\qemu\qemu-system-i386.exe

REM Check if kernel exists
if not exist "build\kernel.bin" (
    echo [!] kernel.bin not found in build directory
    echo [*] Creating minimal bootable kernel...
    
    REM Create a minimal 512-byte bootloader
    REM This is a placeholder - in production you'd use NASM
    echo [!] NASM not available - cannot create bootloader
    echo.
    echo To properly build ZEBX:
    echo 1. Install NASM: https://www.nasm.us
    echo 2. Install i386-elf-gcc: https://osdev.org/GCC_Cross-Compiler
    echo 3. Then rebuild
    echo.
    pause
    exit /b 1 
)

echo [*] Found kernel: build\kernel.bin
echo [*] Launching QEMU...
echo.

REM Boot from floppy disk image
"%QEMU_PATH%" -fda build\kernel.bin -m 256 -name "ZEBX OS" -monitor stdio

pause
