[bits 32]

global _start
extern kmain

section .text
_start:
    cli
    mov esp, stack_top
    push eax            ; multiboot magic
    push ebx            ; multiboot info
    call kmain
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

