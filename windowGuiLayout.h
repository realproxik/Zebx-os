#ifndef WINDOW_H
#define WINDOW_H

#include "kernel/types.h"
#include "kernel/vbe.h"

#define MAX_WINDOWS 10
#define WINDOW_TITLEBAR_HEIGHT 20
#define WINDOW_BORDER_WIDTH 2

typedef struct {
    int x, y;
    int width, height;
    char title[64];
    uint32_t bg_color;
    int active;
    int visible;
    int moving;
    int move_offset_x, move_offset_y;
} Window;

void window_init(void);
Window* window_create(int x, int y, int w, int h, const char *title, uint32_t bg);
void window_destroy(Window *win);
void window_draw(Window *win);
void window_draw_all(void);
void window_move(Window *win, int x, int y);
void window_set_active(Window *win);
Window* window_get_active(void);


static Window windows[MAX_WINDOWS];
static int window_count = 0;
static Window *active_window = NULL;

void window_init(void) {
    window_count = 0;
    active_window = NULL;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].visible = 0;
    }
}

Window* window_create(int x, int y, int w, int h, const char *title, uint32_t bg) {
    if (window_count >= MAX_WINDOWS) return NULL;
    
    Window *win = &windows[window_count++];
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    
    // Copy title
    int i = 0;
    while (title[i] && i < 63) {
        win->title[i] = title[i];
        i++;
    }
    win->title[i] = '\0';
    
    win->bg_color = bg;
    win->active = 0;
    win->visible = 1;
    win->moving = 0;
    
    return win;
}

void window_draw(Window *win) {
    if (!win->visible) return;
    
    // Title bar (active = blue, inactive = gray)
    uint32_t title_color = win->active ? COLOR_BLUE : COLOR_GRAY;
    vbe_fill_rect(win->x, win->y, win->width, WINDOW_TITLEBAR_HEIGHT, title_color);
    
    // Title text (white)
    // vbe_draw_string(win->x + 5, win->y + 5, win->title, COLOR_WHITE, title_color);
    
    // Close button (red X) - top right
    vbe_fill_rect(win->x + win->width - 18, win->y + 2, 16, 16, COLOR_RED);
    
    // Window body
    vbe_fill_rect(win->x + WINDOW_BORDER_WIDTH, 
                  win->y + WINDOW_TITLEBAR_HEIGHT,
                  win->width - WINDOW_BORDER_WIDTH * 2,
                  win->height - WINDOW_TITLEBAR_HEIGHT - WINDOW_BORDER_WIDTH,
                  win->bg_color);
    
    // Border
    vbe_draw_rect(win->x, win->y, win->width, win->height, COLOR_DARK_GRAY);
}

void window_draw_all(void) {
    for (int i = 0; i < window_count; i++) {
        window_draw(&windows[i]);
    }
}

void window_move(Window *win, int x, int y) {
    if (win) {
        win->x = x;
        win->y = y;
    }
}

void window_set_active(Window *win) {
    if (active_window) active_window->active = 0;
    active_window = win;
    if (win) win->active = 1;
}

Window* window_get_active(void) {
    return active_window;
}
#endif