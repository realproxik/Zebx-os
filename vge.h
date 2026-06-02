#ifndef  vbe_h
#ifndef vbe_h

#include "kernel.h/types.h"

typedef struct{
    unint16_t mode;
    unint16_t width;
    unint16_t height;
    unint16_t bpp;
    unint16_t framebuffer;

} vbe_mode_t;

#define COLOR_bLACK
#define COLOR_BLUE
#define COLOR_GREEN
#define COLOR_CYAN
#define COLOR_RED
#define COLOR_MAGENTA
#define COLOR_BROWN
#define COLOR_LIGHT_GREY
#define COLOR_DARK_GREY
#define COLOR_LIGHT_BLUE
#define COLOR_LIGHT_GREEN
#define COLOR_LIGHT_CYAN
#define COLOR_LIGHT_RED
#define COLOR_LIGHT_MAGENTA
#define COLOR_YELLOW
#define COLOR_WHITE
#define COLOR_YELLOW

void vbe_init();
void vbe_put_pixel(int x, int y, uint32_t color)
void vbe_fill_rect(int x, int y, int w, int h, uint32_t color);
void vbe_draw_rect(int x, int y, int w, int h, uint32_t color);
void vbe_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void vbe_clear(uint32_t color);
void vbe_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void vbe_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg);


static vbe_mode_t vbe_mode;
static uint32_t *framebuffer;

void vbe_init(void) {
    // VBE mode 0x4118 = 1024x768x32 (check your hardware)
    // For now, use QEMU's default VBE
    vbe_mode.width = 800;
    vbe_mode.height = 600;
    vbe_mode.bpp = 32;
    vbe_mode.framebuffer = 0xE0000000; // QEMU VBE framebuffer
    
    framebuffer = (uint32_t *)vbe_mode.framebuffer;
    
    // Clear to black
    vbe_clear(COLOR_BLACK);
}

void vbe_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= vbe_mode.width || y < 0 || y >= vbe_mode.height) return;
    framebuffer[y * vbe_mode.width + x] = color;
}

void vbe_fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            vbe_put_pixel(x + dx, y + dy, color);
        }
    }
}

void vbe_draw_rect(int x, int y, int w, int h, uint32_t color) {
    // Top
    for (int dx = 0; dx < w; dx++) vbe_put_pixel(x + dx, y, color);
    // Bottom
    for (int dx = 0; dx < w; dx++) vbe_put_pixel(x + dx, y + h - 1, color);
    // Left
    for (int dy = 0; dy < h; dy++) vbe_put_pixel(x, y + dy, color);
    // Right
    for (int dy = 0; dy < h; dy++) vbe_put_pixel(x + w - 1, y + dy, color);
}

void vbe_clear(uint32_t color) {
    vbe_fill_rect(0, 0, vbe_mode.width, vbe_mode.height, color);
}

void vbe_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    // Simple 8x8 font (bitmap)
    // You'll need to create a font array
    // For now, draw a colored rectangle
    vbe_fill_rect(x, y, 8, 8, fg);
}

#endif
