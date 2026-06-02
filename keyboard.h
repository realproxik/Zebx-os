
# 4. keyboard.h - Keyboard driver header

keyboard_h = '''// ZEBX OS - Keyboard Driver Header
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

void keyboard_init(void);
void keyboard_irq(void);
int keyboard_has_char(void);
char keyboard_get_char(void);
char keyboard_getc(void);
void keyboard_gets(char *buffer, int max_len);

#endif
'''

with open('/mnt/agents/output/zebx_os/kernel/keyboard.h', 'w') as f:
    f.write(keyboard_h)

# 5. keyboard.c - Keyboard driver implementation

keyboard_c = '''// ZEBX OS - PS/2 Keyboard Driver

#include "types.h"
#include "vga.h"
#include "interrupt.h"

#define KEYBOARD_PORT 0x60
#define KEYBOARD_STATUS 0x64
#define KEYBOARD_CMD 0x64
#define KEY_BUFFER_SIZE 256

typedef struct {
    uint8_t buffer[KEY_BUFFER_SIZE];
    int head;
    int tail;
} key_buffer_t;

static key_buffer_t key_buffer = {0};

static uint8_t scancode_ascii[] = {
    0,   27,  '1', '2', '3',  '4', '5',  '6', '7',  '8', '9',  '0', '-',  '=', '\\b', '\\t',
    'q', 'w', 'e', 'r', 't',  'y', 'u',  'i', 'o',  'p', '[',  ']', '\\n', 0,   'a',  's',
    'd', 'f', 'g', 'h', 'j',  'k', 'l',  ';', '\\'', '`', 0,    '\\\\', 'z', 'x', 'c',  'v',
    'b', 'n', 'm', ',', '.',  '/', 0,    '*', 0,    ' ', 0,    0,    0,   0,   0,    0,
    0,   0,   0,   0,   0,    0,   0,    0,   0,    0,   '-',  0,    0,   0,   '+',  0,
    0,   0,   0,   0,   0,    0,   0,    0,   0,    0,   0,    0,    0,   0,   0,    0,
};

static uint8_t scancode_shift[] = {
    0,   27,  '!', '@', '#',  '$', '%',  '^', '&',  '*', '(',  ')', '_',  '+', '\\b', '\\t',
    'Q', 'W', 'E', 'R', 'T',  'Y', 'U',  'I', 'O',  'P', '{',  '}', '\\n', 0,   'A',  'S',
    'D', 'F', 'G', 'H', 'J',  'K', 'L',  ':', '"',  '~', 0,    '|', 'Z', 'X', 'C',  'V',
    'B', 'N', 'M', '<', '>',  '?', 0,    '*', 0,    ' ', 0,    0,    0,   0,   0,    0,
};

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

void keyboard_irq(void) {
    uint8_t status = inb(KEYBOARD_STATUS);
    
    if (!(status & 1)) {
        return;
    }

    uint8_t scancode = inb(KEYBOARD_PORT);
    uint8_t ascii = 0;

    if (scancode & 0x80) {
        scancode &= 0x7F;
        
        if (scancode == 0x36) shift_pressed = 0;
        if (scancode == 0x2A) shift_pressed = 0;
        if (scancode == 0x1D) ctrl_pressed = 0;
        if (scancode == 0x38) alt_pressed = 0;
        
        return;
    }

    if (scancode == 0x36 || scancode == 0x2A) {
        shift_pressed = 1;
        return;
    }

    if (scancode == 0x1D) {
        ctrl_pressed = 1;
        return;
    }

    if (scancode == 0x38) {
        alt_pressed = 1;
        return;
    }

    if (scancode < sizeof(scancode_ascii)) {
        if (shift_pressed && scancode < sizeof(scancode_shift)) {
            ascii = scancode_shift[scancode];
        } else {
            ascii = scancode_ascii[scancode];
        }
    }

    if (ascii) {
        if (ctrl_pressed && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 1;
        }

        int next_head = (key_buffer.head + 1) % KEY_BUFFER_SIZE;
        if (next_head != key_buffer.tail) {
            key_buffer.buffer[key_buffer.head] = ascii;
            key_buffer.head = next_head;
        }
    }
}

static void keyboard_wait(void) {
    uint32_t timeout = 100000;
    while (timeout-- && (inb(KEYBOARD_STATUS) & 2));
}

void keyboard_init(void) {
    key_buffer.head = 0;
    key_buffer.tail = 0;
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;

    irq_register_handler(IRQ_KEYBOARD, keyboard_irq);

    keyboard_wait();
    outb(0xAA, KEYBOARD_CMD);
    keyboard_wait();

    keyboard_wait();
    outb(0xAE, KEYBOARD_CMD);
    keyboard_wait();

    outb(0xF3, KEYBOARD_PORT);
    keyboard_wait();
    outb(0x00, KEYBOARD_PORT);
    keyboard_wait();
}

int keyboard_has_char(void) {
    return key_buffer.head != key_buffer.tail;
}

char keyboard_get_char(void) {
    if (key_buffer.head == key_buffer.tail) {
        return 0;
    }

    char ch = key_buffer.buffer[key_buffer.tail];
    key_buffer.tail = (key_buffer.tail + 1) % KEY_BUFFER_SIZE;
    return ch;
}

char keyboard_getc(void) {
    while (!keyboard_has_char()) {
        asm volatile("hlt");
    }
    return keyboard_get_char();
}

void keyboard_gets(char *buffer, int max_len) {
    int pos = 0;
    
    while (pos < max_len - 1) {
        char ch = keyboard_getc();
        
        if (ch == '\\n' || ch == '\\r') {
            vga_putchar('\\n');
            buffer[pos] = '\\0';
            break;
        } else if (ch == '\\b') {
            if (pos > 0) {
                pos--;
                vga_putchar('\\b');
                vga_putchar(' ');
                vga_putchar('\\b');
            }
        } else {
            buffer[pos++] = ch;
            vga_putchar(ch);
        }
    }
    
    buffer[pos] = '\\0';
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outb(uint8_t val, uint16_t port) {
    asm volatile("outb %0, %1" : : "a"(val), "dN"(port));
}