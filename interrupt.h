#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

void idt_init(void);
void idt_set_exception(uint8_t exception_num, uint32_t handler_addr);
void idt_set_irq(uint8_t irq_num, uint32_t handler_addr);
void irq_register_handler(uint8_t irq, void (*handler)(void));
void exception_handler(uint32_t exception_num, uint32_t error_code, uint32_t eip);
void irq_handler(uint32_t irq_num, uint32_t regs);

#endif
'''

with open('/mnt/agents/output/zebx_os/kernel/interrupt.h', 'w') as f:
    f.write(interrupt_h)

# 7. interrupt.c - Interrupt handler implementation

interrupt_c = '''// ZEBX OS - Interrupt Descriptor Table (IDT)

#include "types.h"
#include "vga.h"

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t idt_reg;

static void (*irq_handlers[16])(void) = {0};

const char *exception_names[] = {
    "Division by Zero",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
};

void exception_handler(uint32_t exception_num, uint32_t error_code, uint32_t eip) {
    vga_set_color(VGA_RED, VGA_BLACK);
    vga_puts("\\n[EXCEPTION] ");
    
    if (exception_num < 20) {
        vga_puts(exception_names[exception_num]);
    } else {
        vga_puts("Unknown");
    }
    
    vga_puts(" (");
    vga_put_int(exception_num);
    vga_puts(")\\n");
    
    if (exception_num == 14) {
        uint32_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        vga_puts("Address: 0x");
        vga_put_hex(cr2);
        vga_puts("\\n");
    }
    
    vga_puts("EIP: 0x");
    vga_put_hex(eip);
    vga_puts("\\n");
    
    vga_set_color(VGA_WHITE, VGA_BLACK);
    
    asm volatile("cli");
    while(1) {
        asm volatile("hlt");
    }
}

void irq_handler(uint32_t irq_num, uint32_t regs) {
    if (irq_num < 16 && irq_handlers[irq_num]) {
        irq_handlers[irq_num]();
    }

    if (irq_num >= 8) {
        __asm__("outb $0x20, $0xA0");
    }
    __asm__("outb $0x20, $0x20");
}

static void idt_set_entry(uint8_t index, uint32_t base, uint16_t selector, uint8_t type) {
    idt[index].offset_low = base & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = type;
    idt[index].offset_high = (base >> 16) & 0xFFFF;
}

void idt_set_exception(uint8_t exception_num, uint32_t handler_addr) {
    idt_set_entry(exception_num, handler_addr, 0x08, 0x8E);
}

void idt_set_irq(uint8_t irq_num, uint32_t handler_addr) {
    idt_set_entry(32 + irq_num, handler_addr, 0x08, 0x8E);
}

void irq_register_handler(uint8_t irq, void (*handler)(void)) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void idt_init(void) {
    idt_reg.limit = sizeof(idt) - 1;
    idt_reg.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
    }

    extern void isr0(void), isr1(void), isr2(void), isr3(void), isr4(void);
    extern void isr5(void), isr6(void), isr7(void), isr8(void), isr9(void);
    extern void isr10(void), isr11(void), isr12(void), isr13(void), isr14(void);
    extern void isr15(void), isr16(void), isr17(void), isr18(void), isr19(void);

    idt_set_exception(0, (uint32_t)isr0);
    idt_set_exception(1, (uint32_t)isr1);
    idt_set_exception(2, (uint32_t)isr2);
    idt_set_exception(3, (uint32_t)isr3);
    idt_set_exception(4, (uint32_t)isr4);
    idt_set_exception(5, (uint32_t)isr5);
    idt_set_exception(6, (uint32_t)isr6);
    idt_set_exception(7, (uint32_t)isr7);
    idt_set_exception(8, (uint32_t)isr8);
    idt_set_exception(9, (uint32_t)isr9);
    idt_set_exception(10, (uint32_t)isr10);
    idt_set_exception(11, (uint32_t)isr11);
    idt_set_exception(12, (uint32_t)isr12);
    idt_set_exception(13, (uint32_t)isr13);
    idt_set_exception(14, (uint32_t)isr14);
    idt_set_exception(15, (uint32_t)isr15);
    idt_set_exception(16, (uint32_t)isr16);
    idt_set_exception(17, (uint32_t)isr17);
    idt_set_exception(18, (uint32_t)isr18);
    idt_set_exception(19, (uint32_t)isr19);

    extern void irq0(void), irq1(void), irq2(void), irq3(void), irq4(void);
    extern void irq5(void), irq6(void), irq7(void), irq8(void), irq9(void);
    extern void irq10(void), irq11(void), irq12(void), irq13(void), irq14(void), irq15(void);

    idt_set_irq(0, (uint32_t)irq0);
    idt_set_irq(1, (uint32_t)irq1);
    idt_set_irq(2, (uint32_t)irq2);
    idt_set_irq(3, (uint32_t)irq3);
    idt_set_irq(4, (uint32_t)irq4);
    idt_set_irq(5, (uint32_t)irq5);
    idt_set_irq(6, (uint32_t)irq6);
    idt_set_irq(7, (uint32_t)irq7);
    idt_set_irq(8, (uint32_t)irq8);
    idt_set_irq(9, (uint32_t)irq9);
    idt_set_irq(10, (uint32_t)irq10);
    idt_set_irq(11, (uint32_t)irq11);
    idt_set_irq(12, (uint32_t)irq12);
    idt_set_irq(13, (uint32_t)irq13);
    idt_set_irq(14, (uint32_t)irq14);
    idt_set_irq(15, (uint32_t)irq15);

    __asm__("lidt %0" : : "m"(idt_reg));

    __asm__("outb $0x11, $0x20");
    __asm__("outb $0x20, $0x21");
    __asm__("outb $0x04, $0x21");
    __asm__("outb $0x01, $0x21");

    __asm__("outb $0x11, $0xA0");
    __asm__("outb $0x28, $0xA1");
    __asm__("outb $0x02, $0xA1");
    __asm__("outb $0x01, $0xA1");

    __asm__("outb $0x00, $0x21");
    __asm__("outb $0x00, $0xA1");
}