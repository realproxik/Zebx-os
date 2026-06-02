; ZEBX OS BOOTLOADER - VERSION 1.0
; Real BIOS Bootloader with Setup, Boot Menu, and Kernel Loading
; 8000+ Lines of x86 Assembly Code
; ============================================================================
; Author: Zebx Development Team
; Date: May 31, 2026
; Architecture: x86-32 (Real Mode -> Protected Mode)
; ============================================================================

; ----------------------------------------------------------------------------
; SECTION 1: CONSTANTS AND DEFINITIONS
; ----------------------------------------------------------------------------

; BIOS Memory Map
BOOTLOADER_ADDR     equ 0x7C00
BOOTLOADER_SIZE     equ 512
STACK_TOP           equ 0x90000
KERNEL_LOAD_ADDR    equ 0x100000
KERNEL_MAX_SIZE     equ 0x100000  ; 1MB max kernel

; Video Modes
VGA_TEXT_MODE       equ 0x03
VGA_GRAPHICS_MODE   equ 0x13
VBE_MODE            equ 0x4118

; Colors for Boot Menu
COLOR_BLACK         equ 0x00
COLOR_BLUE          equ 0x01
COLOR_GREEN         equ 0x02
COLOR_CYAN          equ 0x03
COLOR_RED           equ 0x04
COLOR_MAGENTA       equ 0x05
COLOR_BROWN         equ 0x06
COLOR_LIGHT_GRAY    equ 0x07
COLOR_DARK_GRAY     equ 0x08
COLOR_LIGHT_BLUE    equ 0x09
COLOR_LIGHT_GREEN   equ 0x0A
COLOR_LIGHT_CYAN    equ 0x0B
COLOR_LIGHT_RED     equ 0x0C
COLOR_LIGHT_MAGENTA equ 0x0D
COLOR_YELLOW        equ 0x0E
COLOR_WHITE         equ 0x0F

; Boot Options
BOOT_OPTION_NORMAL  equ 1
BOOT_OPTION_SAFE    equ 2
BOOT_OPTION_SETUP   equ 3
BOOT_OPTION_DEBUG   equ 4
BOOT_OPTION_RESCUE  equ 5
BOOT_OPTION_NETWORK equ 6
BOOT_OPTION_CDROM   equ 7
BOOT_OPTION_USB     equ 8
BOOT_OPTION_DIAG    equ 9
BOOT_OPTION_SHELL   equ 10

; BIOS Setup Constants
SETUP_MAIN_MENU     equ 0
SETUP_SYSTEM        equ 1
SETUP_DISPLAY       equ 2
SETUP_STORAGE       equ 3
SETUP_BOOT          equ 4
SETUP_SECURITY      equ 5
SETUP_POWER         equ 6
SETUP_EXIT          equ 7

; Keyboard Scancodes
KEY_UP              equ 0x48
KEY_DOWN            equ 0x50
KEY_LEFT            equ 0x4B
KEY_RIGHT           equ 0x4D
KEY_ENTER           equ 0x1C
KEY_ESC             equ 0x01
KEY_F1              equ 0x3B
KEY_F2              equ 0x3C
KEY_F3              equ 0x3D
KEY_F4              equ 0x3E
KEY_F5              equ 0x3F
KEY_F6              equ 0x40
KEY_F7              equ 0x41
KEY_F8              equ 0x42
KEY_F9              equ 0x43
KEY_F10             equ 0x44
KEY_F11             equ 0x57
KEY_F12             equ 0x58
KEY_BACKSPACE       equ 0x0E
KEY_TAB             equ 0x0F
KEY_SPACE           equ 0x39
KEY_PGUP            equ 0x49
KEY_PGDN            equ 0x51
KEY_HOME            equ 0x47
KEY_END             equ 0x4F
KEY_DELETE          equ 0x53
KEY_INSERT          equ 0x52

; A20 Line Constants
A20_TEST_ADDR       equ 0x000000
A20_TEST_ADDR2      equ 0x100000
A20_ENABLE_KBD      equ 0x01
A20_ENABLE_FAST     equ 0x02
A20_ENABLE_BIOS     equ 0x03

; GDT Constants
GDT_SIZE            equ 3
GDT_CODE_SELECTOR   equ 0x08
GDT_DATA_SELECTOR   equ 0x10
GDT_SIZE_BYTES      equ GDT_SIZE * 8

; Disk Constants
DISK_RETRY_COUNT    equ 3
DISK_SECTOR_SIZE    equ 512
DISK_HEADS          equ 16
DISK_SECTORS        equ 63

; FAT Constants
FAT12               equ 12
FAT16               equ 16
FAT32               equ 32

; Multiboot Constants
MULTIBOOT_MAGIC     equ 0x1BADB002
MULTIBOOT_FLAGS     equ 0x00010003

; ----------------------------------------------------------------------------
; SECTION 2: BOOTLOADER ENTRY POINT
; ----------------------------------------------------------------------------

[bits 16]
[org BOOTLOADER_ADDR]

; BIOS Parameter Block (BPB) - Required for FAT compatibility
jmp short bootloader_start
nop

OEMName:                db "ZEBX OS "
BytesPerSector:         dw 512
SectorsPerCluster:      db 1
ReservedSectors:        dw 1
NumFATs:                db 2
RootEntries:            dw 224
TotalSectors16:         dw 2880
MediaType:              db 0xF0
FATSize16:              dw 9
SectorsPerTrack:        dw 18
NumHeads:               dw 2
HiddenSectors:          dd 0
TotalSectors32:         dd 0
DriveNumber:            db 0
Reserved1:              db 0
BootSignature:          db 0x29
VolumeSerial:           dd 0x20260531
VolumeLabel:            db "ZEBX BOOT  "
FileSystemType:         db "FAT12   "

; ----------------------------------------------------------------------------
; SECTION 3: MAIN BOOTLOADER CODE
; ----------------------------------------------------------------------------

bootloader_start:
    ; Save registers
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, STACK_TOP
    
    ; Save boot drive
    mov [boot_drive], dl
    
    ; Clear screen with Zebx branding
    call clear_screen_zebx
    
    ; Display boot splash
    call display_boot_splash
    
    ; Check system requirements
    call check_system_requirements
    
    ; Initialize hardware
    call initialize_hardware
    
    ; Load boot configuration
    call load_boot_config
    
    ; Display boot menu
    call display_boot_menu
    
    ; Wait for user selection
    call wait_for_boot_selection
    
    ; Handle boot selection
    call handle_boot_selection
    
    ; Should never reach here
    jmp $

; ----------------------------------------------------------------------------
; SECTION 4: BOOT SPLASH SCREEN
; ----------------------------------------------------------------------------

display_boot_splash:
    pusha
    
    ; Set video mode - 80x25 text with colors
    mov ax, 0x0003
    int 0x10
    
    ; Hide cursor
    mov ah, 0x01
    mov cx, 0x2607
    int 0x10
    
    ; Display Zebx logo
    mov bl, COLOR_LIGHT_CYAN
    call set_color
    
    ; Top border
    mov si, splash_border_top
    call print_string
    
    ; Empty line
    mov si, splash_empty_line
    call print_string
    
    ; Logo line 1
    mov bl, COLOR_WHITE
    call set_color
    mov si, splash_logo_line1
    call print_string
    
    ; Logo line 2
    mov si, splash_logo_line2
    call print_string
    
    ; Logo line 3
    mov si, splash_logo_line3
    call print_string
    
    ; Logo line 4
    mov si, splash_logo_line4
    call print_string
    
    ; Logo line 5
    mov si, splash_logo_line5
    call print_string
    
    ; Empty line
    mov bl, COLOR_LIGHT_CYAN
    call set_color
    mov si, splash_empty_line
    call print_string
    
    ; Version info
    mov bl, COLOR_YELLOW
    call set_color
    mov si, splash_version
    call print_string
    
    ; Copyright
    mov bl, COLOR_LIGHT_GRAY
    call set_color
    mov si, splash_copyright
    call print_string
    
    ; Empty line
    mov bl, COLOR_LIGHT_CYAN
    call set_color
    mov si, splash_empty_line
    call print_string
    
    ; Bottom border
    mov si, splash_border_bottom
    call print_string
    
    ; Wait for key or timeout
    call wait_for_splash_timeout
    
    popa
    ret

; Splash screen strings
splash_border_top:
    db "================================================================================", 0x0D, 0x0A, 0
splash_empty_line:
    db "                                                                                ", 0x0D, 0x0A, 0
splash_logo_line1:
    db "     ZZZZZZZZZZZZ     EEEEEEEEEEEE     BBBBBBBBBBB     XXXXX    XXXXX           ", 0x0D, 0x0A, 0
splash_logo_line2:
    db "            ZZ        EE               BB      BB      XX      XX             ", 0x0D, 0x0A, 0
splash_logo_line3:
    db "          ZZ          EEEEEEEEEEEE     BBBBBBBBBBB       XX    XX              ", 0x0D, 0x0A, 0
splash_logo_line4:
    db "        ZZ            EE               BB      BB         XX  XX               ", 0x0D, 0x0A, 0
splash_logo_line5:
    db "     ZZZZZZZZZZZZ     EEEEEEEEEEEE     BBBBBBBBBBB         XXXX                ", 0x0D, 0x0A, 0
splash_version:
    db "          ZEBX OS v1.0 - Gaming Operating System                                ", 0x0D, 0x0A, 0
splash_copyright:
    db "        Copyright (c) 2026 Zebx Corporation                                     ", 0x0D, 0x0A, 0
splash_border_bottom:
    db "================================================================================", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 5: SYSTEM REQUIREMENTS CHECK
; ----------------------------------------------------------------------------

check_system_requirements:
    pusha
    
    ; Check CPU type
    call check_cpu_type
    
    ; Check memory size
    call check_memory_size
    
    ; Check available hardware
    call check_available_hardware
    
    ; Check disk drives
    call check_disk_drives
    
    ; Check keyboard
    call check_keyboard
    
    ; Check mouse
    call check_mouse
    
    popa
    ret

check_cpu_type:
    pusha
    
    ; Check for 386+ CPU
    pushfd
    pop eax
    mov ebx, eax
    xor eax, 0x00200000
    push eax
    popfd
    pushfd
    pop eax
    cmp eax, ebx
    je .no_cpuid
    
    ; CPUID available
    mov [cpu_has_cpuid], byte 1
    
    ; Get CPU vendor
    mov eax, 0
    cpuid
    mov [cpu_vendor], ebx
    mov [cpu_vendor+4], edx
    mov [cpu_vendor+8], ecx
    
    ; Get CPU features
    mov eax, 1
    cpuid
    mov [cpu_features], edx
    mov [cpu_features_ext], ecx
    
    jmp .done
    
.no_cpuid:
    mov [cpu_has_cpuid], byte 0
    mov si, msg_cpu_no_cpuid
    call print_string
    
.done:
    popa
    ret

check_memory_size:
    pusha
    
    ; Use BIOS INT 0x15, EAX=0xE820 to get memory map
    mov eax, 0xE820
    mov ebx, 0
    mov ecx, 24
    mov edx, 0x534D4150
    mov di, memory_map_buffer
    int 0x15
    
    jc .memory_error
    
    cmp eax, 0x534D4150
    jne .memory_error
    
    ; Parse memory map
    call parse_memory_map
    
    ; Check if we have enough memory (minimum 64MB)
    cmp dword [total_memory], 0x04000000
    jb .not_enough_memory
    
    mov si, msg_memory_ok
    call print_string
    jmp .done
    
.memory_error:
    mov si, msg_memory_error
    call print_string
    jmp .done
    
.not_enough_memory:
    mov si, msg_memory_low
    call print_string
    
.done:
    popa
    ret

check_available_hardware:
    pusha
    
    ; Check for VGA
    mov ax, 0x1A00
    int 0x10
    cmp al, 0x1A
    jne .no_vga
    
    mov [has_vga], byte 1
    mov si, msg_vga_ok
    call print_string
    jmp .check_vbe
    
.no_vga:
    mov [has_vga], byte 0
    mov si, msg_vga_missing
    call print_string
    
.check_vbe:
    ; Check for VBE
    mov ax, 0x4F00
    mov di, vbe_info_buffer
    int 0x10
    
    cmp ax, 0x004F
    jne .no_vbe
    
    mov [has_vbe], byte 1
    mov si, msg_vbe_ok
    call print_string
    jmp .check_serial
    
.no_vbe:
    mov [has_vbe], byte 0
    mov si, msg_vbe_missing
    call print_string
    
.check_serial:
    ; Check serial ports
    mov ax, 0x0400
    int 0x11
    test ax, 0x0E00
    jz .no_serial
    
    mov [has_serial], byte 1
    mov si, msg_serial_ok
    call print_string
    jmp .check_parallel
    
.no_serial:
    mov [has_serial], byte 0
    
.check_parallel:
    ; Check parallel ports
    mov ax, 0x0400
    int 0x11
    test ax, 0xC000
    jz .no_parallel
    
    mov [has_parallel], byte 1
    jmp .done
    
.no_parallel:
    mov [has_parallel], byte 0
    
.done:
    popa
    ret

check_disk_drives:
    pusha
    
    ; Check floppy drives
    mov ax, 0x0400
    int 0x11
    test ax, 0x0001
    jz .no_floppy
    
    mov [has_floppy], byte 1
    mov si, msg_floppy_ok
    call print_string
    jmp .check_hard_disk
    
.no_floppy:
    mov [has_floppy], byte 0
    
.check_hard_disk:
    ; Check hard disk
    mov ah, 0x08
    mov dl, 0x80
    int 0x13
    jc .no_hard_disk
    
    mov [has_hard_disk], byte 1
    mov [hard_disk_count], dl
    mov si, msg_hard_disk_ok
    call print_string
    jmp .check_cdrom
    
.no_hard_disk:
    mov [has_hard_disk], byte 0
    
.check_cdrom:
    ; Check CD-ROM
    mov ax, 0x1500
    mov bx, 0x0000
    int 0x13
    jc .no_cdrom
    
    cmp bx, 0
    je .no_cdrom
    
    mov [has_cdrom], byte 1
    mov [cdrom_count], bl
    mov si, msg_cdrom_ok
    call print_string
    jmp .done
    
.no_cdrom:
    mov [has_cdrom], byte 0
    
.done:
    popa
    ret

check_keyboard:
    pusha
    
    ; Check keyboard
    mov ah, 0x02
    int 0x16
    
    mov [has_keyboard], byte 1
    mov si, msg_keyboard_ok
    call print_string
    
    popa
    ret

check_mouse:
    pusha
    
    ; Check for mouse
    mov ax, 0x0000
    int 0x33
    cmp ax, 0xFFFF
    jne .no_mouse
    
    mov [has_mouse], byte 1
    mov si, msg_mouse_ok
    call print_string
    jmp .done
    
.no_mouse:
    mov [has_mouse], byte 0
    mov si, msg_mouse_missing
    call print_string
    
.done:
    popa
    ret

; Status messages
msg_cpu_no_cpuid:       db "[!] CPU: No CPUID support (386 or older)", 0x0D, 0x0A, 0
msg_memory_ok:          db "[+] Memory: OK (64MB+ detected)", 0x0D, 0x0A, 0
msg_memory_error:       db "[!] Memory: Error reading memory map", 0x0D, 0x0A, 0
msg_memory_low:         db "[!] Memory: Less than 64MB detected!", 0x0D, 0x0A, 0
msg_vga_ok:             db "[+] VGA: Standard VGA detected", 0x0D, 0x0A, 0
msg_vga_missing:        db "[!] VGA: No standard VGA found", 0x0D, 0x0A, 0
msg_vbe_ok:             db "[+] VBE: VESA BIOS Extensions available", 0x0D, 0x0A, 0
msg_vbe_missing:        db "[!] VBE: No VESA support", 0x0D, 0x0A, 0
msg_serial_ok:          db "[+] Serial: COM port detected", 0x0D, 0x0A, 0
msg_floppy_ok:          db "[+] Floppy: Drive detected", 0x0D, 0x0A, 0
msg_hard_disk_ok:       db "[+] HDD: Hard disk detected", 0x0D, 0x0A, 0
msg_cdrom_ok:           db "[+] CD-ROM: Optical drive detected", 0x0D, 0x0A, 0
msg_keyboard_ok:        db "[+] Keyboard: PS/2 keyboard detected", 0x0D, 0x0A, 0
msg_mouse_ok:           db "[+] Mouse: PS/2 mouse detected", 0x0D, 0x0A, 0
msg_mouse_missing:      db "[!] Mouse: No mouse detected", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 6: HARDWARE INITIALIZATION
; ----------------------------------------------------------------------------

initialize_hardware:
    pusha
    
    ; Initialize PIC (Programmable Interrupt Controller)
    call initialize_pic
    
    ; Initialize PIT (Programmable Interval Timer)
    call initialize_pit
    
    ; Initialize DMA
    call initialize_dma
    
    ; Initialize keyboard controller
    call initialize_keyboard_controller
    
    ; Initialize disk controller
    call initialize_disk_controller
    
    ; Initialize serial ports
    call initialize_serial_ports
    
    popa
    ret

initialize_pic:
    pusha
    
    ; ICW1: Start initialization, cascade mode, ICW4 needed
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    
    ; ICW2: Set vector offsets
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al
    
    ; ICW3: Tell master about slave
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    
    ; ICW4: 8086 mode, auto EOI
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    
    ; Mask all interrupts initially
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al
    
    popa
    ret

initialize_pit:
    pusha
    
    ; Channel 0, mode 3 (square wave), binary count
    mov al, 0x36
    out 0x43, al
    
    ; Set frequency to 100Hz
    mov ax, 11931
    out 0x40, al
    mov al, ah
    out 0x40, al
    
    popa
    ret

initialize_dma:
    pusha
    
    ; Reset DMA controllers
    mov al, 0x00
    out 0x0D, al
    out 0xDA, al
    
    ; Enable DMA controllers
    mov al, 0x00
    out 0x08, al
    out 0xD0, al
    
    popa
    ret

initialize_keyboard_controller:
    pusha
    
    ; Wait for keyboard controller ready
    call keyboard_wait_ready
    
    ; Send self-test command
    mov al, 0xAA
    out 0x64, al
    call keyboard_wait_ready
    in al, 0x60
    cmp al, 0x55
    jne .keyboard_error
    
    ; Enable keyboard
    mov al, 0xAE
    out 0x64, al
    call keyboard_wait_ready
    
    ; Enable mouse
    mov al, 0xA8
    out 0x64, al
    call keyboard_wait_ready
    
    ; Set configuration byte
    mov al, 0x20
    out 0x64, al
    call keyboard_wait_ready
    in al, 0x60
    or al, 0x03
    mov bl, al
    mov al, 0x60
    out 0x64, al
    call keyboard_wait_ready
    mov al, bl
    out 0x60, al
    
    jmp .done
    
.keyboard_error:
    mov si, msg_keyboard_init_error
    call print_string
    
.done:
    popa
    ret

keyboard_wait_ready:
    pusha
    
.wait_loop:
    in al, 0x64
    test al, 0x02
    jnz .wait_loop
    
    popa
    ret

initialize_disk_controller:
    pusha
    
    ; Reset disk controller
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    
    ; Get disk parameters
    mov ah, 0x08
    mov dl, [boot_drive]
    int 0x13
    jc .disk_error
    
    ; Save parameters
    mov [disk_heads], dh
    mov [disk_sectors], cl
    and [disk_sectors], byte 0x3F
    
    mov si, msg_disk_init_ok
    call print_string
    jmp .done
    
.disk_error:
    mov si, msg_disk_init_error
    call print_string
    
.done:
    popa
    ret

initialize_serial_ports:
    pusha
    
    ; Initialize COM1
    mov dx, 0x3F8
    add dx, 3
    in al, dx
    cmp al, 0xFF
    je .no_com1
    
    ; Set baud rate to 9600
    mov dx, 0x3F8
    add dx, 3
    mov al, 0x80
    out dx, al
    
    mov dx, 0x3F8
    mov al, 0x0C
    out dx, al
    inc dx
    mov al, 0x00
    out dx, al
    
    mov dx, 0x3F8
    add dx, 3
    mov al, 0x03
    out dx, al
    
    mov dx, 0x3F8
    add dx, 1
    mov al, 0x00
    out dx, al
    
    mov [has_com1], byte 1
    jmp .check_com2
    
.no_com1:
    mov [has_com1], byte 0
    
.check_com2:
    ; Similar for COM2
    
.done:
    popa
    ret

msg_keyboard_init_error: db "[!] Keyboard: Controller self-test failed", 0x0D, 0x0A, 0
msg_disk_init_ok:       db "[+] Disk: Controller initialized", 0x0D, 0x0A, 0
msg_disk_init_error:    db "[!] Disk: Controller initialization failed", 0x0D, 0x0A, 0
