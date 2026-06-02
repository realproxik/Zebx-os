#include "kernel/desktop_gui.h"
#include "kernel/vbe.h"
#include "kernel/window.h"
#include "kernel/taskbar.h"

void desktop_gui_init(void) {
    vbe_init();
    window_init();
    taskbar_init();
    
    // Create some default windows
    window_create(50, 50, 300, 200, "Terminal", COLOR_BLACK);
    window_create(400, 100, 250, 150, "Files", COLOR_WHITE);
    
    // Set first window active
    window_set_active(&windows[0]);
}

void desktop_gui_draw(void) {
    // Draw desktop background (blue gradient)
    vbe_clear(COLOR_BLUE);
    
    // Draw desktop icons
    // My Computer icon
    vbe_fill_rect(20, 50, 48, 48, COLOR_GRAY);
    // vbe_draw_string(20, 100, "My Computer", COLOR_WHITE, COLOR_BLUE);
    
    // Files icon
    vbe_fill_rect(20, 130, 48, 48, COLOR_YELLOW);
    // vbe_draw_string(20, 180, "Files", COLOR_WHITE, COLOR_BLUE);
    
    // Settings icon
    vbe_fill_rect(20, 210, 48, 48, COLOR_CYAN);
    // vbe_draw_string(20, 260, "Settings", COLOR_WHITE, COLOR_BLUE);
    
    // Draw all windows
    window_draw_all();
    
    // Draw taskbar (on top)
    taskbar_draw();
}

void desktop_gui_handle_click(int x, int y, int button) {
    // Check taskbar
    if (taskbar_click(x, y)) {
        // Start menu clicked!
        // Show start menu
        return;
    }
    
    // Check windows (reverse order - top first)
    for (int i = window_count - 1; i >= 0; i--) {
        Window *win = &windows[i];
        if (!win->visible) continue;
        
        if (x >= win->x && x < win->x + win->width &&
            y >= win->y && y < win->y + win->height) {
            
            window_set_active(win);
            
            // Check close button
            if (x >= win->x + win->width - 18 && x < win->x + win->width - 2 &&
                y >= win->y + 2 && y < win->y + 18) {
                win->visible = 0; // Close window
                return;
            }
            
            // Check title bar for move
            if (y < win->y + WINDOW_TITLEBAR_HEIGHT) {
                win->moving = 1;
                win->move_offset_x = x - win->x;
                win->move_offset_y = y - win->y;
            }
            
            return;
        }
    }
}

void desktop_gui_handle_mouse_move(int x, int y) {
    for (int i = 0; i < window_count; i++) {
        Window *win = &windows[i];
        if (win->moving) {
            window_move(win, x - win->move_offset_x, y - win->move_offset_y);
        }
    }
}