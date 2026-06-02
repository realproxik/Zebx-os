#ifndef MOUSE_GUI_H
#define MOUSE_GUI_H

#include "kernel/mouse_gui.h"
#include "kernel/vbe.h"
// start with a simple software cursor. Later we can add hardware cursor support if needed.
void mouse_cursor_init(void);
void mouse_cursor_draw(int x, int y);
void mouse_cursor_hide(int x, int y);


// Simple arrow cursor (8x12)
static uint8_t cursor_bitmap[12] = {
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111000,
    0b11110000,
    0b11100000,
    0b11000000,
    0b10000000,
    0b00000000
};

void mouse_cursor_draw(int x, int y) {
    for (int dy = 0; dy < 12; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            if (cursor_bitmap[dy] & (0x80 >> dx)) {
                vbe_put_pixel(x + dx, y + dy, COLOR_WHITE);
            }
        }
    }
}

void mouse_cursor_hide(int x, int y) {
    // Redraw background at cursor position
    // (Need to store background pixels)
}
#endif