#ifndef DESKTOP_GUI_H
#define DESKTOP_GUI_H

#include "kernel/types.h"

void desktop_gui_init(void);
void desktop_gui_draw(void);
void desktop_gui_handle_click(int x, int y, int button);
void desktop_gui_handle_mouse_move(int x, int y);

#endif