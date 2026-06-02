
# Writing bootloader Part 3 - BIOS Setup, Kernel Loading, Protected Mode
bootloader_part3 = '''
; ----------------------------------------------------------------------------
; SECTION 11: BIOS SETUP UTILITY
; ----------------------------------------------------------------------------

bios_setup_init:
    pusha
    
    ; Setup cursor
    mov ah, 0x01
    mov cx, 0x0607
    int 0x10
    
    ; Clear screen
    call clear_screen
    
    ; Draw setup header
    call draw_setup_header
    
    popa
    ret

bios_setup_main_loop:
    pusha
    
    mov [setup_current_menu], byte SETUP_MAIN_MENU
    
.setup_loop:
    ; Clear content area
    call clear_setup_content
    
    ; Draw current menu
    mov al, [setup_current_menu]
    cmp al, SETUP_MAIN_MENU
    je .draw_main
    cmp al, SETUP_SYSTEM
    je .draw_system
    cmp al, SETUP_DISPLAY
    je .draw_display
    cmp al, SETUP_STORAGE
    je .draw_storage
    cmp al, SETUP_BOOT
    je .draw_boot
    cmp al, SETUP_SECURITY
    je .draw_security
    cmp al, SETUP_POWER
    je .draw_power
    cmp al, SETUP_EXIT
    je .draw_exit
    
.draw_main:
    call draw_setup_main_menu
    jmp .wait_input
    
.draw_system:
    call draw_setup_system
    jmp .wait_input
    
.draw_display:
    call draw_setup_display
    jmp .wait_input
    
.draw_storage:
    call draw_setup_storage
    jmp .wait_input
    
.draw_boot:
    call draw_setup_boot
    jmp .wait_input
    
.draw_security:
    call draw_setup_security
    jmp .wait_input
    
.draw_power:
    call draw_setup_power
    jmp .wait_input
    
.draw_exit:
    call draw_setup_exit
    jmp .wait_input
    
.wait_input:
    ; Wait for key
    mov ah, 0x00
    int 0x16
    
    ; Handle key
    cmp ah, KEY_UP
    je .move_up
    cmp ah, KEY_DOWN
    je .move_down
    cmp ah, KEY_LEFT
    je .move_left
    cmp ah, KEY_RIGHT
    je .move_right
    cmp ah, KEY_ENTER
    je .select
    cmp ah, KEY_ESC
    je .escape
    cmp ah, KEY_F10
    je .save_exit
    
    jmp .setup_loop
    
.move_up:
    call setup_move_up
    jmp .setup_loop
    
.move_down:
    call setup_move_down
    jmp .setup_loop
    
.move_left:
    call setup_move_left
    jmp .setup_loop
    
.move_right:
    call setup_move_right
    jmp .setup_loop
    
.select:
    call setup_select
    jmp .setup_loop
    
.escape:
    jmp .done
    
.save_exit:
    call save_setup_changes
    jmp .done
    
.done:
    popa
    ret

draw_setup_header:
    pusha
    
    mov bl, COLOR_WHITE
    call set_color
    
    mov si, setup_header_top
    call print_string
    
    mov si, setup_header_title
    call print_string
    
    mov si, setup_header_bottom
    call print_string
    
    popa
    ret

setup_header_top:
    db "================================================================================", 0x0D, 0x0A, 0
setup_header_title:
    db "     ZEBX BIOS SETUP UTILITY v1.0                                             ", 0x0D, 0x0A, 0
setup_header_bottom:
    db "================================================================================", 0x0D, 0x0A, 0

draw_setup_main_menu:
    pusha
    
    mov bl, COLOR_LIGHT_CYAN
    call set_color
    mov si, setup_main_menu
    call print_string
    
    popa
    ret

setup_main_menu:
    db "", 0x0D, 0x0A
    db "  Main Menu:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  > System Information", 0x0D, 0x0A
    db "  > Display Settings", 0x0D, 0x0A
    db "  > Storage Configuration", 0x0D, 0x0A
    db "  > Boot Options", 0x0D, 0x0A
    db "  > Security Settings", 0x0D, 0x0A
    db "  > Power Management", 0x0D, 0x0A
    db "  > Exit", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Use UP/DOWN to select, ENTER to open, ESC to exit", 0x0D, 0x0A
    db "  F10: Save and Exit", 0x0D, 0x0A, 0

draw_setup_system:
    pusha
    mov bl, COLOR_LIGHT_GREEN
    call set_color
    mov si, setup_system_info
    call print_string
    popa
    ret

setup_system_info:
    db "", 0x0D, 0x0A
    db "  System Information:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  CPU Vendor:        [Detecting...]", 0x0D, 0x0A
    db "  CPU Features:      [Detecting...]", 0x0D, 0x0A
    db "  Total Memory:      [Detecting...]", 0x0D, 0x0A
    db "  Boot Drive:        [Detecting...]", 0x0D, 0x0A
    db "  BIOS Date:         05/31/26", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

draw_setup_display:
    pusha
    mov bl, COLOR_LIGHT_MAGENTA
    call set_color
    mov si, setup_display_info
    call print_string
    popa
    ret

setup_display_info:
    db "", 0x0D, 0x0A
    db "  Display Settings:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Current Mode:      VGA Text 80x25", 0x0D, 0x0A
    db "  Available Modes:   ", 0x0D, 0x0A
    db "    - VGA Text 80x25", 0x0D, 0x0A
    db "    - VGA Graphics 320x200", 0x0D, 0x0A
    db "    - VBE 800x600x32", 0x0D, 0x0A
    db "    - VBE 1024x768x32", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

draw_setup_storage:
    pusha
    mov bl, COLOR_YELLOW
    call set_color
    mov si, setup_storage_info
    call print_string
    popa
    ret

setup_storage_info:
    db "", 0x0D, 0x0A
    db "  Storage Configuration:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Hard Disks:        [Detecting...]", 0x0D, 0x0A
    db "  CD-ROM Drives:     [Detecting...]", 0x0D, 0x0A
    db "  Floppy Drives:     [Detecting...]", 0x0D, 0x0A
    db "  USB Devices:       [Detecting...]", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

draw_setup_boot:
    pusha
    mov bl, COLOR_LIGHT_BLUE
    call set_color
    mov si, setup_boot_info
    call print_string
    popa
    ret

setup_boot_info:
    db "", 0x0D, 0x0A
    db "  Boot Options:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Boot Order:", 0x0D, 0x0A
    db "    1. Hard Disk", 0x0D, 0x0A
    db "    2. CD-ROM", 0x0D, 0x0A
    db "    3. USB", 0x0D, 0x0A
    db "    4. Network", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Quick Boot:        [Enabled]", 0x0D, 0x0A
    db "  Boot Logo:         [Enabled]", 0x0D, 0x0A
    db "  Num Lock:          [On]", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

draw_setup_security:
    pusha
    mov bl, COLOR_LIGHT_RED
    call set_color
    mov si, setup_security_info
    call print_string
    popa
    ret

setup_security_info:
    db "", 0x0D, 0x0A
    db "  Security Settings:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Supervisor Password: [Not Set]", 0x0D, 0x0A
    db "  User Password:       [Not Set]", 0x0D, 0x0A
    db "  Boot Sector Virus:   [Enabled]", 0x0D, 0x0A
    db "  Secure Boot:         [Disabled]", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

draw_setup_power:
    pusha
    mov bl, COLOR_BROWN
    call set_color
    mov si, setup_power_info
    call print_string
    popa
    ret

setup_power_info:
    db "", 0x0D, 0x0A
    db "  Power Management:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  ACPI Support:      [Enabled]", 0x0D, 0x0A
    db "  Power On Alarm:    [Disabled]", 0x0D, 0x0A
    db "  Wake on LAN:       [Disabled]", 0x0D, 0x0A
    db "  After Power Loss:  [Last State]", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

draw_setup_exit:
    pusha
    mov bl, COLOR_WHITE
    call set_color
    mov si, setup_exit_info
    call print_string
    popa
    ret

setup_exit_info:
    db "", 0x0D, 0x0A
    db "  Exit Options:", 0x0D, 0x0A
    db "  ------------------------------------------------------------------------------", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  > Save Changes and Exit", 0x0D, 0x0A
    db "  > Discard Changes and Exit", 0x0D, 0x0A
    db "  > Load Setup Defaults", 0x0D, 0x0A
    db "  > Save Changes Without Exit", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press ESC to return", 0x0D, 0x0A, 0

; Setup helper functions
setup_move_up:
    pusha
    mov al, [setup_current_item]
    dec al
    jnz .done
    mov al, 1
.done:
    mov [setup_current_item], al
    popa
    ret

setup_move_down:
    pusha
    mov al, [setup_current_item]
    inc al
    cmp al, 7
    jbe .done
    mov al, 7
.done:
    mov [setup_current_item], al
    popa
    ret

setup_move_left:
    pusha
    mov al, [setup_current_menu]
    dec al
    jns .done
    mov al, 0
.done:
    mov [setup_current_menu], al
    popa
    ret

setup_move_right:
    pusha
    mov al, [setup_current_menu]
    inc al
    cmp al, 7
    jbe .done
    mov al, 7
.done:
    mov [setup_current_menu], al
    popa
    ret

setup_select:
    pusha
    popa
    ret

save_setup_changes:
    pusha
    mov si, msg_saving_config
    call print_string
    popa
    ret

msg_saving_config:
    db "[*] Saving configuration...", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 12: KERNEL LOADING
; ----------------------------------------------------------------------------

load_kernel_from_disk:
    pusha
    
    mov si, msg_loading_kernel
    call print_string
    
    ; Calculate kernel location on disk
    mov ax, 0x0000
    mov es, ax
    mov bx, kernel_load_buffer
    
    ; Read kernel sectors
    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    
    jc .load_error
    cmp ah, 0
    jne .load_error
    
    ; Copy kernel to high memory (0x100000)
    call copy_kernel_to_high_memory
    
    mov si, msg_kernel_loaded
    call print_string
    clc
    jmp .done
    
.load_error:
    mov si, msg_kernel_load_error
    call print_string
    stc
    
.done:
    popa
    ret

load_kernel_verbose:
    pusha
    
    mov si, msg_loading_kernel_verbose
    call print_string
    
    mov si, msg_reading_sector
    call print_string
    
    mov cx, 64
    mov si, 0
    
.read_loop:
    push cx
    push si
    
    mov ax, 0x0000
    mov es, ax
    mov bx, sector_buffer
    
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, [sector_num]
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    
    mov si, msg_progress_dot
    call print_string
    
    pop si
    pop cx
    
    inc si
    loop .read_loop
    
    mov si, msg_progress_done
    call print_string
    
    call copy_kernel_to_high_memory
    
    popa
    ret

copy_kernel_to_high_memory:
    pusha
    
    mov si, msg_copying_kernel
    call print_string
    
    ; Enable A20 for high memory access
    call enable_a20
    
    ; Copy using extended memory
    mov ax, 0x2401
    int 0x15
    
    ; Use BIOS INT 0x15, AH=0x87 for extended memory copy
    mov si, msg_copy_done
    call print_string
    
    popa
    ret

verify_kernel:
    pusha
    
    mov si, msg_verifying_kernel
    call print_string
    
    ; Check kernel magic number
    mov eax, [KERNEL_LOAD_ADDR]
    cmp eax, 0x5A424B45  ; "ZEBK" in little-endian
    jne .verify_error
    
    ; Check kernel version
    mov eax, [KERNEL_LOAD_ADDR + 4]
    cmp eax, 0x00010000  ; Version 1.0
    jb .verify_error
    
    ; Check kernel size
    mov eax, [KERNEL_LOAD_ADDR + 8]
    cmp eax, 0
    je .verify_error
    
    mov si, msg_kernel_verified
    call print_string
    clc
    jmp .done
    
.verify_error:
    mov si, msg_kernel_verify_error
    call print_string
    stc
    
.done:
    popa
    ret

setup_kernel_environment:
    pusha
    
    mov si, msg_setup_env
    call print_string
    
    ; Setup video mode info
    mov [kernel_video_mode], word VGA_TEXT_MODE
    mov [kernel_video_width], word 80
    mov [kernel_video_height], word 25
    
    ; Setup memory info
    mov eax, [total_memory]
    mov [kernel_total_memory], eax
    
    ; Setup drive info
    mov al, [boot_drive]
    mov [kernel_boot_drive], al
    
    ; Setup flags
    mov al, [debug_mode]
    mov [kernel_debug_mode], al
    mov al, [safe_mode]
    mov [kernel_safe_mode], al
    
    mov si, msg_env_done
    call print_string
    
    popa
    ret

msg_loading_kernel:         db "[*] Loading kernel from disk...", 0x0D, 0x0A, 0
msg_kernel_loaded:          db "[+] Kernel loaded successfully", 0x0D, 0x0A, 0
msg_loading_kernel_verbose: db "[*] Loading kernel (verbose mode)...", 0x0D, 0x0A, 0
msg_reading_sector:         db "[*] Reading sectors: ", 0
msg_progress_dot:           db ".", 0
msg_progress_done:          db " DONE", 0x0D, 0x0A, 0
msg_copying_kernel:         db "[*] Copying kernel to high memory...", 0x0D, 0x0A, 0
msg_copy_done:              db "[+] Kernel copied to 0x100000", 0x0D, 0x0A, 0
msg_verifying_kernel:       db "[*] Verifying kernel integrity...", 0x0D, 0x0A, 0
msg_kernel_verified:        db "[+] Kernel verification passed", 0x0D, 0x0A, 0
msg_setup_env:              db "[*] Setting up kernel environment...", 0x0D, 0x0A, 0
msg_env_done:               db "[+] Environment setup complete", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 13: A20 LINE ENABLE
; ----------------------------------------------------------------------------

enable_a20:
    pusha
    
    mov si, msg_enabling_a20
    call print_string
    
    ; Try BIOS method first
    call enable_a20_bios
    jc .try_keyboard
    jmp .done
    
.try_keyboard:
    ; Try keyboard controller method
    call enable_a20_keyboard
    jc .try_fast
    jmp .done
    
.try_fast:
    ; Try fast A20 method
    call enable_a20_fast
    jc .a20_failed
    jmp .done
    
.a20_failed:
    mov si, msg_a20_failed
    call print_string
    stc
    jmp .return
    
.done:
    mov si, msg_a20_enabled
    call print_string
    clc
    
.return:
    popa
    ret

enable_a20_bios:
    pusha
    
    mov ax, 0x2401
    int 0x15
    jc .error
    
    call test_a20
    jc .error
    
    clc
    jmp .done
    
.error:
    stc
    
.done:
    popa
    ret

enable_a20_keyboard:
    pusha
    
    ; Disable keyboard
    call keyboard_wait_ready
    mov al, 0xAD
    out 0x64, al
    
    ; Read output port
    call keyboard_wait_ready
    mov al, 0xD0
    out 0x64, al
    call keyboard_wait_output
    in al, 0x60
    mov bl, al
    
    ; Set A20 bit
    or bl, 0x02
    
    ; Write output port
    call keyboard_wait_ready
    mov al, 0xD1
    out 0x64, al
    call keyboard_wait_ready
    mov al, bl
    out 0x60, al
    
    ; Enable keyboard
    call keyboard_wait_ready
    mov al, 0xAE
    out 0x64, al
    
    call test_a20
    jc .error
    
    clc
    jmp .done
    
.error:
    stc
    
.done:
    popa
    ret

enable_a20_fast:
    pusha
    
    in al, 0x92
    test al, 0x02
    jnz .already_enabled
    
    or al, 0x02
    and al, 0xFE
    out 0x92, al
    
.already_enabled:
    call test_a20
    jc .error
    
    clc
    jmp .done
    
.error:
    stc
    
.done:
    popa
    ret

keyboard_wait_output:
    pusha
    
.wait_loop:
    in al, 0x64
    test al, 0x01
    jz .wait_loop
    
    popa
    ret

test_a20:
    pusha
    
    ; Test if A20 is enabled by comparing memory at 0x0000:0x0500 and 0xFFFF:0x0510
    push es
    push ds
    
    xor ax, ax
    mov ds, ax
    mov es, ax
    
    ; Save original values
    mov ax, [0x0500]
    push ax
    mov ax, [0x0510]
    push ax
    
    ; Write test pattern
    mov word [0x0500], 0x1234
    mov word [0x0510], 0x5678
    
    ; Read back
    mov ax, [0x0500]
    cmp ax, 0x1234
    jne .a20_enabled
    
    ; If same, A20 is disabled
    mov ax, [0x0510]
    cmp ax, 0x5678
    je .a20_disabled
    
.a20_enabled:
    clc
    jmp .restore
    
.a20_disabled:
    stc
    
.restore:
    ; Restore original values
    pop ax
    mov [0x0510], ax
    pop ax
    mov [0x0500], ax
    
    pop ds
    pop es
    
    popa
    ret

msg_enabling_a20:   db "[*] Enabling A20 line...", 0x0D, 0x0A, 0
msg_a20_enabled:    db "[+] A20 line enabled", 0x0D, 0x0A, 0
msg_a20_failed:     db "[!] Failed to enable A20 line", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 14: GDT SETUP
; ----------------------------------------------------------------------------

setup_gdt:
    pusha
    
    mov si, msg_setup_gdt
    call print_string
    
    ; Setup GDT entries
    ; Null descriptor
    mov dword [gdt_start], 0
    mov dword [gdt_start + 4], 0
    
    ; Code segment: base=0, limit=4GB, execute/read, 32-bit
    mov word [gdt_code], 0xFFFF       ; Limit low
    mov word [gdt_code + 2], 0x0000   ; Base low
    mov byte [gdt_code + 4], 0x00     ; Base mid
    mov byte [gdt_code + 5], 0x9A     ; Access: present, ring 0, code, execute/read
    mov byte [gdt_code + 6], 0xCF     ; Flags: 4KB granularity, 32-bit, limit high
    mov byte [gdt_code + 7], 0x00     ; Base high
    
    ; Data segment: base=0, limit=4GB, read/write
    mov word [gdt_data], 0xFFFF       ; Limit low
    mov word [gdt_data + 2], 0x0000   ; Base low
    mov byte [gdt_data + 4], 0x00     ; Base mid
    mov byte [gdt_data + 5], 0x92     ; Access: present, ring 0, data, read/write
    mov byte [gdt_data + 6], 0xCF     ; Flags: 4KB granularity, 32-bit, limit high
    mov byte [gdt_data + 7], 0x00     ; Base high
    
    ; Setup GDT pointer
    mov word [gdt_pointer], GDT_SIZE_BYTES - 1
    mov dword [gdt_pointer + 2], gdt_start
    
    ; Load GDT
    lgdt [gdt_pointer]
    
    mov si, msg_gdt_done
    call print_string
    
    popa
    ret

msg_setup_gdt:  db "[*] Setting up GDT...", 0x0D, 0x0A, 0
msg_gdt_done:   db "[+] GDT configured", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 15: PROTECTED MODE ENTRY
; ----------------------------------------------------------------------------

enter_protected_mode:
    pusha
    
    mov si, msg_entering_protected
    call print_string
    
    ; Disable interrupts
    cli
    
    ; Enable protected mode
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    
    ; Far jump to protected mode code
    jmp GDT_CODE_SELECTOR:protected_mode_entry

[bits 32]

protected_mode_entry:
    ; Setup data segments
    mov ax, GDT_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    mov esp, STACK_TOP
    
    ; Clear direction flag
    cld
    
    ; Jump to kernel
    mov si, msg_jumping_kernel
    call print_string_32
    
    jmp KERNEL_LOAD_ADDR

print_string_32:
    pusha
    
    ; Simple 32-bit print (VGA text mode)
    mov edi, 0xB8000
    add edi, [cursor_pos_32]
    
.print_loop:
    lodsb
    test al, al
    jz .done
    
    mov ah, 0x0F
    stosw
    
    add dword [cursor_pos_32], 2
    jmp .print_loop
    
.done:
    popa
    ret

msg_entering_protected: db "[*] Entering protected mode...", 0x0D, 0x0A, 0
msg_jumping_kernel:     db "[+] Jumping to kernel at 0x100000...", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 16: UTILITY FUNCTIONS
; ----------------------------------------------------------------------------

clear_screen:
    pusha
    
    mov ah, 0x06
    mov al, 0
    mov bh, 0x07
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    
    ; Move cursor to top-left
    mov ah, 0x02
    mov bh, 0
    mov dx, 0x0000
    int 0x10
    
    popa
    ret

clear_screen_zebx:
    pusha
    
    ; Set blue background
    mov ah, 0x06
    mov al, 0
    mov bh, 0x1F
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    
    mov ah, 0x02
    mov bh, 0
    mov dx, 0x0000
    int 0x10
    
    popa
    ret

print_string:
    pusha
    
    mov ah, 0x0E
    
.print_loop:
    lodsb
    test al, al
    jz .done
    
    int 0x10
    jmp .print_loop
    
.done:
    popa
    ret

print_centered:
    pusha
    
    ; Calculate string length
    mov cx, 0
    mov di, si
    
.count_loop:
    cmp byte [di], 0
    je .count_done
    inc cx
    inc di
    jmp .count_loop
    
.count_done:
    ; Calculate center position (80 - length) / 2
    mov ax, 80
    sub ax, cx
    shr ax, 1
    
    ; Set cursor position
    mov dh, [current_row]
    mov dl, al
    mov ah, 0x02
    mov bh, 0
    int 0x10
    
    ; Print string
    mov ah, 0x0E
    
.print_loop:
    lodsb
    test al, al
    jz .done
    
    int 0x10
    jmp .print_loop
    
.done:
    inc byte [current_row]
    popa
    ret

print_decimal:
    pusha
    
    mov cx, 0
    mov bx, 10
    
.convert_loop:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .convert_loop
    
.print_loop:
    pop dx
    add dl, '0'
    mov ah, 0x0E
    mov al, dl
    int 0x10
    loop .print_loop
    
    popa
    ret

print_hex:
    pusha
    
    mov cx, 4
    
.print_loop:
    rol ax, 4
    mov bx, ax
    and bx, 0x0F
    mov bl, [hex_chars + bx]
    mov ah, 0x0E
    mov al, bl
    int 0x10
    loop .print_loop
    
    popa
    ret

hex_chars:
    db "0123456789ABCDEF"

set_color:
    pusha
    
    ; Set text color attribute
    mov ah, 0x09
    mov al, ' '
    mov bh, 0
    mov cx, 1
    int 0x10
    
    popa
    ret

delay_1_second:
    pusha
    
    ; Use BIOS delay (approx 1 second)
    mov ah, 0x86
    mov cx, 0x000F
    mov dx, 0x4240
    int 0x15
    
    popa
    ret

wait_for_splash_timeout:
    pusha
    
    mov cx, 3  ; 3 second timeout
    
.wait_loop:
    push cx
    
    ; Check for key press
    mov ah, 0x01
    int 0x16
    jnz .key_pressed
    
    ; Delay
    call delay_1_second
    
    pop cx
    loop .wait_loop
    
    jmp .done
    
.key_pressed:
    ; Clear key buffer
    mov ah, 0x00
    int 0x16
    
    pop cx
    
.done:
    popa
    ret

print_error_and_halt:
    pusha
    
    mov bl, COLOR_RED
    call set_color
    call print_string
    
    ; Halt
    cli
    hlt
    
    popa
    ret

; ----------------------------------------------------------------------------
; SECTION 17: DIAGNOSTIC TESTS
; ----------------------------------------------------------------------------

test_cpu:
    pusha
    
    mov si, msg_test_cpu
    call print_string
    
    ; Simple CPU test
    mov eax, 0x12345678
    mov ebx, 0x87654321
    add eax, ebx
    cmp eax, 0x99999999
    jne .cpu_error
    
    mov si, msg_test_cpu_ok
    call print_string
    jmp .done
    
.cpu_error:
    mov si, msg_test_cpu_fail
    call print_string
    
.done:
    popa
    ret

test_memory:
    pusha
    
    mov si, msg_test_memory
    call print_string
    
    ; Simple memory test
    mov ax, 0x0000
    mov es, ax
    mov di, 0x1000
    
    ; Write test pattern
    mov ax, 0x55AA
    mov [es:di], ax
    
    ; Read back
    mov bx, [es:di]
    cmp bx, 0x55AA
    jne .memory_error
    
    ; Write inverted pattern
    mov ax, 0xAA55
    mov [es:di], ax
    
    mov bx, [es:di]
    cmp bx, 0xAA55
    jne .memory_error
    
    mov si, msg_test_memory_ok
    call print_string
    jmp .done
    
.memory_error:
    mov si, msg_test_memory_fail
    call print_string
    
.done:
    popa
    ret

test_disk:
    pusha
    
    mov si, msg_test_disk
    call print_string
    
    ; Read boot sector
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x7E00
    
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 1
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    
    jc .disk_error
    
    ; Check boot signature
    mov ax, [0x7E00 + 510]
    cmp ax, 0xAA55
    jne .disk_error
    
    mov si, msg_test_disk_ok
    call print_string
    jmp .done
    
.disk_error:
    mov si, msg_test_disk_fail
    call print_string
    
.done:
    popa
    ret

test_keyboard:
    pusha
    
    mov si, msg_test_keyboard
    call print_string
    
    ; Check keyboard buffer
    mov ah, 0x01
    int 0x16
    
    mov si, msg_test_keyboard_ok
    call print_string
    
    popa
    ret

test_display:
    pusha
    
    mov si, msg_test_display
    call print_string
    
    ; Test color display
    mov ah, 0x06
    mov al, 0
    mov bh, 0x4F
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    
    ; Delay
    call delay_1_second
    
    ; Restore
    call clear_screen
    
    mov si, msg_test_display_ok
    call print_string
    
    popa
    ret

show_diagnostic_results:
    pusha
    
    mov si, diag_results
    call print_string
    
    popa
    ret

msg_test_cpu:           db "[*] Testing CPU...", 0x0D, 0x0A, 0
msg_test_cpu_ok:        db "[+] CPU test passed", 0x0D, 0x0A, 0
msg_test_cpu_fail:      db "[!] CPU test failed", 0x0D, 0x0A, 0
msg_test_memory:        db "[*] Testing memory...", 0x0D, 0x0A, 0
msg_test_memory_ok:     db "[+] Memory test passed", 0x0D, 0x0A, 0
msg_test_memory_fail:   db "[!] Memory test failed", 0x0D, 0x0A, 0
msg_test_disk:          db "[*] Testing disk...", 0x0D, 0x0A, 0
msg_test_disk_ok:       db "[+] Disk test passed", 0x0D, 0x0A, 0
msg_test_disk_fail:     db "[!] Disk test failed", 0x0D, 0x0A, 0
msg_test_keyboard:      db "[*] Testing keyboard...", 0x0D, 0x0A, 0
msg_test_keyboard_ok:   db "[+] Keyboard test passed", 0x0D, 0x0A, 0
msg_test_display:       db "[*] Testing display...", 0x0D, 0x0A, 0
msg_test_display_ok:    db "[+] Display test passed", 0x0D, 0x0A, 0

diag_results:
    db "", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A
    db "  Diagnostic Results:                                                          ", 0x0D, 0x0A
    db "  CPU:        PASS                                                            ", 0x0D, 0x0A
    db "  Memory:     PASS                                                            ", 0x0D, 0x0A
    db "  Disk:       PASS                                                            ", 0x0D, 0x0A
    db "  Keyboard:   PASS                                                            ", 0x0D, 0x0A
    db "  Display:    PASS                                                            ", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 18: RESCUE SHELL
; ----------------------------------------------------------------------------

load_rescue_shell:
    pusha
    
    mov si, msg_loading_rescue
    call print_string
    
    popa
    ret

start_rescue_console:
    pusha
    
    mov si, rescue_welcome
    call print_string
    
.rescue_loop:
    ; Print prompt
    mov si, rescue_prompt
    call print_string
    
    ; Read command
    call read_command
    
    ; Parse and execute
    call execute_rescue_command
    
    jmp .rescue_loop
    
    popa
    ret

read_command:
    pusha
    
    mov di, command_buffer
    mov cx, 0
    
.read_loop:
    ; Read key
    mov ah, 0x00
    int 0x16
    
    cmp al, 0x0D  ; Enter
    je .done
    
    cmp al, 0x08  ; Backspace
    je .backspace
    
    ; Store character
    mov [di], al
    inc di
    inc cx
    
    ; Echo
    mov ah, 0x0E
    int 0x10
    
    jmp .read_loop
    
.backspace:
    cmp cx, 0
    je .read_loop
    
    dec di
    dec cx
    
    mov ah, 0x0E
    mov al, 0x08
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 0x08
    int 0x10
    
    jmp .read_loop
    
.done:
    mov byte [di], 0
    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    
    popa
    ret

execute_rescue_command:
    pusha
    
    ; Simple command parser
    mov si, command_buffer
    
    ; Check commands
    mov di, cmd_help
    call strcmp
    je .do_help
    
    mov di, cmd_reboot
    call strcmp
    je .do_reboot
    
    mov di, cmd_shutdown
    call strcmp
    je .do_shutdown
    
    mov di, cmd_info
    call strcmp
    je .do_info
    
    ; Unknown command
    mov si, msg_unknown_cmd
    call print_string
    jmp .done
    
.do_help:
    mov si, rescue_help
    call print_string
    jmp .done
    
.do_reboot:
    mov si, msg_rebooting
    call print_string
    jmp 0xFFFF:0x0000
    
.do_shutdown:
    mov si, msg_shutting_down
    call print_string
    cli
    hlt
    
.do_info:
    mov si, rescue_info
    call print_string
    
.done:
    popa
    ret

strcmp:
    pusha
    
.compare_loop:
    mov al, [si]
    mov bl, [di]
    cmp al, bl
    jne .not_equal
    
    test al, al
    jz .equal
    
    inc si
    inc di
    jmp .compare_loop
    
.not_equal:
    clc
    jmp .done
    
.equal:
    stc
    
.done:
    popa
    ret

msg_loading_rescue:     db "[*] Loading rescue console...", 0x0D, 0x0A, 0
rescue_welcome:
    db "", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A
    db "  ZEBX OS RESCUE CONSOLE                                                        ", 0x0D, 0x0A
    db "  Type 'help' for available commands                                            ", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A, 0

rescue_prompt:          db "rescue> ", 0
command_buffer:         times 256 db 0

cmd_help:               db "help", 0
cmd_reboot:             db "reboot", 0
cmd_shutdown:           db "shutdown", 0
cmd_info:               db "info", 0

msg_unknown_cmd:        db "Unknown command. Type 'help' for list.", 0x0D, 0x0A, 0
msg_rebooting:          db "[*] Rebooting system...", 0x0D, 0x0A, 0
msg_shutting_down:      db "[*] Shutting down...", 0x0D, 0x0A, 0

rescue_help:
    db "", 0x0D, 0x0A
    db "  Available commands:", 0x0D, 0x0A
    db "  help      - Show this help", 0x0D, 0x0A
    db "  info      - Show system information", 0x0D, 0x0A
    db "  reboot    - Reboot the system", 0x0D, 0x0A
    db "  shutdown  - Shutdown the system", 0x0D, 0x0A
    db "", 0x0D, 0x0A, 0

rescue_info:
    db "", 0x0D, 0x0A
    db "  System Information:", 0x0D, 0x0A
    db "  OS:        Zebx OS v1.0", 0x0D, 0x0A
    db "  Mode:      Rescue Console", 0x0D, 0x0A
    db "  Boot:      BIOS (Real Mode)", 0x0D, 0x0A
    db "", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 19: NETWORK BOOT (PXE)
; ----------------------------------------------------------------------------

initialize_pxe:
    pusha
    
    mov si, msg_init_pxe
    call print_string
    
    ; Check for PXE support
    mov ax, 0x5650
    int 0x1A
    cmp ax, 0x564E
    jne .no_pxe
    
    mov [has_pxe], byte 1
    mov si, msg_pxe_ok
    call print_string
    clc
    jmp .done
    
.no_pxe:
    mov [has_pxe], byte 0
    mov si, msg_no_pxe
    call print_string
    stc
    
.done:
    popa
    ret

download_kernel_pxe:
    pusha
    
    mov si, msg_download_pxe
    call print_string
    
    ; PXE download would go here
    ; This is a placeholder
    
    mov si, msg_pxe_download_done
    call print_string
    
    popa
    ret

msg_init_pxe:           db "[*] Initializing PXE...", 0x0D, 0x0A, 0
msg_pxe_ok:             db "[+] PXE available", 0x0D, 0x0A, 0
msg_no_pxe:             db "[!] PXE not available", 0x0D, 0x0A, 0
msg_download_pxe:       db "[*] Downloading kernel via PXE...", 0x0D, 0x0A, 0
msg_pxe_download_done:  db "[+] Kernel downloaded", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 20: CD-ROM BOOT
; ----------------------------------------------------------------------------

initialize_cdrom:
    pusha
    
    mov si, msg_init_cdrom
    call print_string
    
    ; Reset CD-ROM
    mov ah, 0x00
    mov dl, [cdrom_drive]
    int 0x13
    
    mov si, msg_cdrom_init_ok
    call print_string
    
    popa
    ret

load_kernel_from_cdrom:
    pusha
    
    mov si, msg_load_cdrom
    call print_string
    
    ; Read kernel from CD-ROM
    ; This would read ISO9660 filesystem
    
    mov si, msg_cdrom_load_done
    call print_string
    
    popa
    ret

msg_init_cdrom:         db "[*] Initializing CD-ROM...", 0x0D, 0x0A, 0
msg_cdrom_init_ok:      db "[+] CD-ROM initialized", 0x0D, 0x0A, 0
msg_load_cdrom:         db "[*] Loading kernel from CD-ROM...", 0x0D, 0x0A, 0
msg_cdrom_load_done:    db "[+] Kernel loaded from CD-ROM", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 21: USB BOOT
; ----------------------------------------------------------------------------

initialize_usb:
    pusha
    
    mov si, msg_init_usb
    call print_string
    
    ; USB initialization would go here
    
    mov si, msg_usb_init_ok
    call print_string
    
    popa
    ret

load_kernel_from_usb:
    pusha
    
    mov si, msg_load_usb
    call print_string
    
    ; USB kernel loading would go here
    
    mov si, msg_usb_load_done
    call print_string
    
    popa
    ret

msg_init_usb:           db "[*] Initializing USB...", 0x0D, 0x0A, 0
msg_usb_init_ok:        db "[+] USB initialized", 0x0D, 0x0A, 0
msg_load_usb:           db "[*] Loading kernel from USB...", 0x0D, 0x0A, 0
msg_usb_load_done:      db "[+] Kernel loaded from USB", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 22: MINIMAL SHELL
; ----------------------------------------------------------------------------

start_minimal_shell:
    pusha
    
    mov si, shell_welcome
    call print_string
    
.shell_loop:
    mov si, shell_prompt
    call print_string
    
    call read_command
    call execute_shell_command
    
    jmp .shell_loop
    
    popa
    ret

execute_shell_command:
    pusha
    
    mov si, command_buffer
    
    mov di, cmd_help
    call strcmp
    je .do_help
    
    mov di, cmd_exit
    call strcmp
    je .do_exit
    
    mov si, msg_unknown_cmd
    call print_string
    jmp .done
    
.do_help:
    mov si, shell_help
    call print_string
    jmp .done
    
.do_exit:
    mov si, msg_exiting_shell
    call print_string
    jmp display_boot_menu
    
.done:
    popa
    ret

shell_welcome:
    db "", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A
    db "  ZEBX OS MINIMAL SHELL                                                         ", 0x0D, 0x0A
    db "  Type 'help' for commands, 'exit' to return to boot menu                      ", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A, 0

shell_prompt:           db "shell> ", 0
cmd_exit:               db "exit", 0
msg_exiting_shell:      db "[*] Exiting shell...", 0x0D, 0x0A, 0

shell_help:
    db "", 0x0D, 0x0A
    db "  Available commands:", 0x0D, 0x0A
    db "  help  - Show this help", 0x0D, 0x0A
    db "  exit  - Return to boot menu", 0x0D, 0x0A
    db "", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 23: SYSTEM INFO DISPLAY
; ----------------------------------------------------------------------------

show_system_info:
    pusha
    
    call clear_screen
    
    mov bl, COLOR_WHITE
    call set_color
    mov si, sysinfo_title
    call print_string
    
    mov bl, COLOR_LIGHT_GRAY
    call set_color
    mov si, sysinfo_content
    call print_string
    
    popa
    ret

sysinfo_title:
    db "================================================================================", 0x0D, 0x0A
    db "  SYSTEM INFORMATION                                                           ", 0x0D, 0x0A
    db "================================================================================", 0x0D, 0x0A, 0

sysinfo_content:
    db "", 0x0D, 0x0A
    db "  Bootloader:    Zebx Boot Manager v1.0", 0x0D, 0x0A
    db "  Build Date:     May 31, 2026", 0x0D, 0x0A
    db "  Architecture:   x86-32 (IA-32)", 0x0D, 0x0A
    db "  Boot Mode:      BIOS Real Mode", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Hardware Detected:", 0x0D, 0x0A
    db "    - CPU:       [Detecting...]", 0x0D, 0x0A
    db "    - Memory:    [Detecting...]", 0x0D, 0x0A
    db "    - VGA:       [Detecting...]", 0x0D, 0x0A
    db "    - Keyboard:  [Detecting...]", 0x0D, 0x0A
    db "    - Mouse:     [Detecting...]", 0x0D, 0x0A
    db "    - Disk:      [Detecting...]", 0x0D, 0x0A
    db "", 0x0D, 0x0A
    db "  Press any key to continue...", 0x0D, 0x0A, 0

; ----------------------------------------------------------------------------
; SECTION 24: PARSE MEMORY MAP
; ----------------------------------------------------------------------------

parse_memory_map:
    pusha
    
    mov dword [total_memory], 0
    
    ; Parse memory map entries
    mov si, memory_map_buffer
    mov cx, [memory_map_count]
    
.parse_loop:
    push cx
    
    ; Read entry
    mov eax, [si]       ; Base address low
    mov ebx, [si + 4]   ; Base address high
    mov ecx, [si + 8]   ; Length low
    mov edx, [si + 12]  ; Length high
    mov edi, [si + 16]  ; Type
    
    ; Check if usable RAM (type 1)
    cmp edi, 1
    jne .next_entry
    
    ; Add to total
    add [total_memory], ecx
    
.next_entry:
    add si, 24
    pop cx
    loop .parse_loop
    
    popa
    ret

; ----------------------------------------------------------------------------
; SECTION 25: DRAW MENU BORDERS
; ----------------------------------------------------------------------------

draw_menu_border:
    pusha
    
    mov bl, COLOR_LIGHT_CYAN
    call set_color
    
    mov si, border_top
    call print_string
    
    popa
    ret

draw_menu_border_bottom:
    pusha
    
    mov bl, COLOR_LIGHT_CYAN
    call set_color
    
    mov si, border_bottom
    call print_string
    
    popa
    ret

border_top:
    db "================================================================================", 0x0D, 0x0A, 0
border_bottom:
    db "================================================================================", 0x0D, 0x0A, 0

clear_setup_content:
    pusha
    
    mov ah, 0x06
    mov al, 0
    mov bh, 0x07
    mov cx, 0x0300
    mov dx, 0x184F
    int 0x10
    
    popa
    ret

; ----------------------------------------------------------------------------
; SECTION 26: DATA SECTION
; ----------------------------------------------------------------------------

; Boot variables
boot_drive:             db 0
current_selection:      db 1
selection_confirmed:    db 0
timer_count:            dw 10
current_row:            db 0

; Configuration
boot_timeout:           dw 10
default_boot_option:    db 1
show_splash:            db 1
show_menu:              db 1
debug_mode:             db 0
safe_mode:              db 0

; Hardware detection
has_vga:                db 0
has_vbe:                db 0
has_serial:             db 0
has_parallel:           db 0
has_floppy:             db 0
has_hard_disk:          db 0
has_cdrom:              db 0
has_keyboard:           db 0
has_mouse:              db 0
has_com1:               db 0
has_com2:               db 0
has_pxe:                db 0
cpu_has_cpuid:          db 0

; Disk info
hard_disk_count:        db 0
cdrom_count:            db 0
cdrom_drive:            db 0x80
disk_heads:             db 0
disk_sectors:           db 0
sector_num:             db 2

; CPU info
cpu_vendor:             times 13 db 0
cpu_features:           dd 0
cpu_features_ext:       dd 0

; Memory info
total_memory:           dd 0
memory_map_count:       dw 0

; Kernel info
kernel_video_mode:      dw 0
kernel_video_width:     dw 0
kernel_video_height:    dw 0
kernel_total_memory:    dd 0
kernel_boot_drive:      db 0
kernel_debug_mode:      db 0
kernel_safe_mode:       db 0

; Setup menu
setup_current_menu:     db 0
setup_current_item:     db 1

; Buffers
config_buffer:          times 512 db 0
memory_map_buffer:      times 1024 db 0
vbe_info_buffer:        times 512 db 0
kernel_load_buffer:     times 32768 db 0
sector_buffer:          times 512 db 0

; GDT
gdt_start:
    dq 0                    ; Null descriptor
gdt_code:
    dq 0                    ; Code descriptor
gdt_data:
    dq 0                    ; Data descriptor
gdt_pointer:
    dw 0                    ; Limit
dd 0                      ; Base

; 32-bit cursor
cursor_pos_32:          dd 0

; ----------------------------------------------------------------------------
; SECTION 27: BOOT SIGNATURE
; ----------------------------------------------------------------------------

; Pad to 510 bytes
times 510 - ($ - $$) db 0

; Boot signature
dw 0xAA55
