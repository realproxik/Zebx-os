
#ifndef SHELL_H
#define SHELL_H

void shell_init(void);
void shell_main(void);
void shell_handle_key(char key);

#endif



#include "types.h"
#include "vga.h"
#include "keyboard.h"
#include "disk.h"
#include "memory.h"
#include "process.h"
#include "usb.h"

#define COMMAND_BUFFER_SIZE 512
#define MAX_ARGS 32

typedef struct {
    const char *name;
    const char *help;
    void (*handler)(int argc, char **argv);
} command_t;

void cmd_help(int argc, char **argv) {
    vga_puts("\\n--- ZEBX OS Commands ---\\n");
    vga_puts("help          - Show this help\\n");
    vga_puts("clear         - Clear screen\\n");
    vga_puts("echo          - Print text\\n");
    vga_puts("ls            - List files\\n");
    vga_puts("pwd           - Print working directory\\n");
    vga_puts("ps            - List processes\\n");
    vga_puts("memstat       - Memory statistics\\n");
    vga_puts("whoami        - Current user\\n");
    vga_puts("uname         - System information\\n");
    vga_puts("about         - About ZEBX OS\\n");
    vga_puts("reboot        - Reboot system\\n");
    vga_puts("shutdown      - Shutdown system\\n");
    vga_puts("calc          - Simple calculator\\n");
    vga_puts("gamepad       - Show controller state\\n");
    vga_puts("\\n");
}

void cmd_gamepad(int argc, char **argv) {
    (void)argc; (void)argv;

    usb_gamepad_state_t st;
    if (usb_gamepad_get_state(0, &st) != 0 || !st.connected) {
        vga_puts("No gamepad detected. (Tip: plug a USB controller; Microsoft/Xbox VID=0x045E works best)\\n");
        return;
    }

    vga_puts("Gamepad[0]: VID=0x");
    vga_put_hex(st.vendor_id);
    vga_puts(" PID=0x");
    vga_put_hex(st.product_id);
    vga_puts("\\n");

    vga_puts("  Buttons: 0x");
    vga_put_hex(st.buttons);
    vga_puts("  Hat: ");
    vga_put_int(st.hat);
    vga_puts("\\n");

    vga_puts("  LX=");
    vga_put_int(st.lx);
    vga_puts(" LY=");
    vga_put_int(st.ly);
    vga_puts(" RX=");
    vga_put_int(st.rx);
    vga_puts(" RY=");
    vga_put_int(st.ry);
    vga_puts("\\n");

    vga_puts("  LT=");
    vga_put_int(st.lt);
    vga_puts(" RT=");
    vga_put_int(st.rt);
    vga_puts("\\n");
}

void cmd_clear(int argc, char **argv) {
    vga_clear(VGA_BLACK);
    vga_set_color(VGA_GREEN, VGA_BLACK);
    vga_puts("ZEBX OS v1.0\\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
}

void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        vga_puts(argv[i]);
        if (i < argc - 1) {
            vga_putchar(' ');
        }
    }
    vga_puts("\\n");
}

void cmd_ls(int argc, char **argv) {
    vga_puts("[DIR] .\\n");
    vga_puts("[DIR] ..\\n");
    vga_puts("\\n");
}

void cmd_pwd(int argc, char **argv) {
    vga_puts("/\\n");
}

void cmd_ps(int argc, char **argv) {
    process_list();
}

void cmd_memstat(int argc, char **argv) {
    memory_stats();
}

void cmd_whoami(int argc, char **argv) {
    vga_puts("root\\n");
}

void cmd_uname(int argc, char **argv) {
    vga_puts("ZEBX OS 1.0 i386\\n");
}

void cmd_about(int argc, char **argv) {
    vga_set_color(VGA_CYAN, VGA_BLACK);
    vga_puts("\\n");
    vga_puts("================================================================================\\n");
    vga_puts("  ZEBX OS - Gaming Operating System                                            \\n");
    vga_puts("  Version 1.0 - Freedom Edition                                                \\n");
    vga_puts("================================================================================\\n");
    vga_puts("\\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("A modern, real operating system from scratch.\\n");
    vga_puts("Written in x86 Assembly, C, C++\\n");
    vga_puts("Features: Protected Mode Kernel, Paging, Multitasking\\n");
    vga_puts("Copyright 2026 - Zebx Corporation\\n");
    vga_puts("\\n");
    vga_set_color(VGA_GREEN, VGA_BLACK);
    vga_puts(">> ");
    vga_set_color(VGA_WHITE, VGA_BLACK);
}

void cmd_calc(int argc, char **argv) {
    if (argc < 3) {
        vga_puts("Usage: calc <number> <op> <number>\\n");
        vga_puts("Operators: + - * /\\n");
        return;
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[3]);
    char op = argv[2][0];

    int result = 0;
    char *op_name = "?";

    switch (op) {
        case '+':
            result = a + b;
            op_name = "+";
            break;
        case '-':
            result = a - b;
            op_name = "-";
            break;
        case '*':
            result = a * b;
            op_name = "*";
            break;
        case '/':
            if (b != 0) {
                result = a / b;
            }
            op_name = "/";
            break;
    }

    vga_put_int(a);
    vga_puts(" ");
    vga_puts(op_name);
    vga_puts(" ");
    vga_put_int(b);
    vga_puts(" = ");
    vga_put_int(result);
    vga_puts("\\n");
}

void cmd_reboot(int argc, char **argv) {
    vga_puts("\\nRebooting...\\n");
    outb(0xFE, 0x64);
    asm volatile("cli; hlt");
}

void cmd_shutdown(int argc, char **argv) {
    vga_puts("\\nShutting down...\\n");
    asm volatile("cli; hlt");
}

command_t commands[] = {
    {"help", "Show help", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {"echo", "Print text", cmd_echo},
    {"ls", "List files", cmd_ls},
    {"pwd", "Print working directory", cmd_pwd},
    {"ps", "List processes", cmd_ps},
    {"memstat", "Memory statistics", cmd_memstat},
    {"whoami", "Current user", cmd_whoami},
    {"uname", "System info", cmd_uname},
    {"about", "About ZEBX OS", cmd_about},
    {"calc", "Calculator", cmd_calc},
    {"gamepad", "Show controller state", cmd_gamepad},
    {"reboot", "Reboot system", cmd_reboot},
    {"shutdown", "Shutdown system", cmd_shutdown},
    {NULL, NULL, NULL},
};

int parse_command(const char *line, char **argv, int max_args) {
    int argc = 0;
    int in_word = 0;
    char *start = NULL;
    char *buffer = (char *)kmalloc(512);
    strcpy(buffer, line);

    for (int i = 0; buffer[i] && argc < max_args; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\\t') {
            if (in_word) {
                buffer[i] = '\\0';
                argv[argc++] = start;
                in_word = 0;
            }
        } else {
            if (!in_word) {
                start = &buffer[i];
                in_word = 1;
            }
        }
    }

    if (in_word) {
        argv[argc++] = start;
    }

    return argc;
}

void execute_command(const char *line) {
    char *argv[MAX_ARGS];
    int argc = parse_command(line, argv, MAX_ARGS);

    if (argc == 0) {
        return;
    }

    for (int i = 0; commands[i].name; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    vga_puts("Unknown command: ");
    vga_puts(argv[0]);
    vga_puts("\\n");
}

int atoi(const char *str) {
    int num = 0;
    int sign = 1;

    if (*str == '-') {
        sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        num = num * 10 + (*str - '0');
        str++;
    }

    return num * sign;
}

void shell_init(void) {
    // Nothing special needed
}

void shell_main(void) {
    vga_set_color(VGA_GREEN, VGA_BLACK);
    vga_puts("Type 'help' for commands\\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    while (1) {
        vga_set_color(VGA_GREEN, VGA_BLACK);
        vga_puts("zebx> ");
        vga_set_color(VGA_YELLOW, VGA_BLACK);

        char command[COMMAND_BUFFER_SIZE];
        keyboard_gets(command, COMMAND_BUFFER_SIZE);

        vga_set_color(VGA_WHITE, VGA_BLACK);

        if (strlen(command) > 0) {
            execute_command(command);
        }
    }
}

void shell_handle_key(char key) {
    // Handle key for shell
}

static inline void outb(uint8_t val, uint16_t port) {
    asm volatile("outb %0, %1" : : "a"(val), "dN"(port));
}