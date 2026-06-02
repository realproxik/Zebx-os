#ifndef TASKBAR_H
#define TASKBAR_H

#include "kernel/types.h"
#include "kernel/vbe.h"

#define TASKBAR_HEIGHT 30
#define START_BUTTON_WIDTH 60

void taskbar_init(void);
void taskbar_draw(void);
void taskbar_update_clock(void);
int taskbar_click(int x, int y);

static int screen_width = 800;
static int screen_height = 600;

void taskbar_init(void) {
    // Nothing special
}

void taskbar_draw(void) {
    int taskbar_y = screen_height - TASKBAR_HEIGHT;
    
    // Taskbar background (gray)
    vbe_fill_rect(0, taskbar_y, screen_width, TASKBAR_HEIGHT, COLOR_GRAY);
    
    // Start button (green)
    vbe_fill_rect(2, taskbar_y + 2, START_BUTTON_WIDTH, TASKBAR_HEIGHT - 4, COLOR_GREEN);
    // vbe_draw_string(8, taskbar_y + 8, "Start", COLOR_WHITE, COLOR_GREEN);
    
    // Taskbar border (top)
    for (int x = 0; x < screen_width; x++) {
        vbe_put_pixel(x, taskbar_y, COLOR_WHITE);
    }
    
    // System tray area (right side)
    // Clock, volume, etc.
    vbe_fill_rect(screen_width - 100, taskbar_y + 2, 98, TASKBAR_HEIGHT - 4, COLOR_DARK_GRAY);
}

void taskbar_update_clock(void) {
    // Update clock display
    // You'll need a timer/RTC driver
}

int taskbar_click(int x, int y) {
    int taskbar_y = screen_height - TASKBAR_HEIGHT;
    
    if (y >= taskbar_y && y < screen_height) {
        // Check start button
        if (x >= 2 && x < START_BUTTON_WIDTH + 2) {
            return 1; // Start button clicked
        }
    }
    return 0;
}
#endif