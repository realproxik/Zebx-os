// ============================================================================
 // ZEBX OS - file manager this pc written by jatin labs
 // ============================================================================
#include "filemanager.h"
#include "vga.h"
#include "memory.h"

static file_manager_t fm_state;

static uint32_t icon_colors[] = {
    0xFFFFB74D, 0xFFFFB74D, 0xFF90A4AE, 0xFF4FC3F7, 0xFFAB47BC,
    0xFFEF5350, 0xFF66BB6A, 0xFFFFCA28, 0xFF8D6E63, 0xFF42A5F5,
    0xFF5C6BC0, 0xFF26A69A, 0xFFFF7043, 0xFF78909C, 0xFF8BC34A,
    0xFF607D8B, 0xFFEC407A, 0xFF7E57C2, 0xFF9E9E9E, 0xFF9E9E9E,
    0xFF29B6F6, 0xFF26C6DA, 0xFF42A5F5, 0xFF5C6BC0, 0xFF1E88E5,
    0xFF1565C0, 0xFF0D47A1, 0xFF2E7D32, 0xFFAD1457, 0xFFC62828,
    0xFF424242, 0xFF1565C0, 0xFF0D47A1, 0xFF455A64, 0xFF616161,
    0xFF757575, 0xFF00BCD4, 0xFF0097A7,
};

void filemanager_init(void) {
    memset(&fm_state, 0, sizeof(file_manager_t));
    strcpy(fm_state.current_path, "/");
    fm_state.view_mode = FM_VIEW_ICONS;
    fm_state.sort_mode = FM_SORT_NAME;
    fm_state.sort_ascending = 1;
    fm_state.sidebar_w = 200;
    fm_state.toolbar_h = 40;
    fm_state.statusbar_h = 25;
    fm_state.show_logo = 1;
    fm_state.show_logs = 1;
    fm_state.window_w = 800;
    fm_state.window_h = 600;

    strcpy(fm_state.bookmarks[0], "/");
    strcpy(fm_state.bookmark_names[0], "This PC");
    strcpy(fm_state.bookmarks[1], "/home");
    strcpy(fm_state.bookmark_names[1], "Home");
    strcpy(fm_state.bookmarks[2], "/home/Desktop");
    strcpy(fm_state.bookmark_names[2], "Desktop");
    strcpy(fm_state.bookmarks[3], "/home/Documents");
    strcpy(fm_state.bookmark_names[3], "Documents");
    strcpy(fm_state.bookmarks[4], "/home/Downloads");
    strcpy(fm_state.bookmark_names[4], "Downloads");
    strcpy(fm_state.bookmarks[5], "/home/Pictures");
    strcpy(fm_state.bookmark_names[5], "Pictures");
    strcpy(fm_state.bookmarks[6], "/home/Music");
    strcpy(fm_state.bookmark_names[6], "Music");
    strcpy(fm_state.bookmarks[7], "/home/Videos");
    strcpy(fm_state.bookmark_names[7], "Videos");
    fm_state.num_bookmarks = 8;

    filemanager_refresh(&fm_state);
    vga_puts("[+] This PC (File Manager) initialized\\n");
}

file_manager_t* filemanager_get_state(void) { return &fm_state; }

void filemanager_refresh(file_manager_t *fm) {
    fm->num_items = 0;
    memset(fm->selected, 0, sizeof(fm->selected));
    fm->num_selected = 0;

    if (!strcmp(fm->current_path, "/")) {
        file_entry_t *e = &fm->items[fm->num_items++];
        strcpy(e->name, "This PC"); strcpy(e->path, "/");
        e->is_directory = 1; e->icon = ICON_COMPUTER; e->color = icon_colors[ICON_COMPUTER];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Desktop"); strcpy(e->path, "/home/Desktop");
        e->is_directory = 1; e->icon = ICON_DESKTOP; e->color = icon_colors[ICON_DESKTOP];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Documents"); strcpy(e->path, "/home/Documents");
        e->is_directory = 1; e->icon = ICON_DOCUMENTS; e->color = icon_colors[ICON_DOCUMENTS];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Downloads"); strcpy(e->path, "/home/Downloads");
        e->is_directory = 1; e->icon = ICON_DOWNLOADS; e->color = icon_colors[ICON_DOWNLOADS];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Pictures"); strcpy(e->path, "/home/Pictures");
        e->is_directory = 1; e->icon = ICON_PICTURES; e->color = icon_colors[ICON_PICTURES];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Music"); strcpy(e->path, "/home/Music");
        e->is_directory = 1; e->icon = ICON_MUSIC; e->color = icon_colors[ICON_MUSIC];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Videos"); strcpy(e->path, "/home/Videos");
        e->is_directory = 1; e->icon = ICON_VIDEOS; e->color = icon_colors[ICON_VIDEOS];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "3GB Storage (ZEBXFS)"); strcpy(e->path, "/storage");
        e->is_directory = 1; e->size = 3ULL * 1024 * 1024 * 1024;
        e->icon = ICON_DRIVE; e->color = icon_colors[ICON_DRIVE];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Network"); strcpy(e->path, "/network");
        e->is_directory = 1; e->icon = ICON_NETWORK; e->color = icon_colors[ICON_NETWORK];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Recycle Bin"); strcpy(e->path, "/trash");
        e->is_directory = 1; e->icon = ICON_RECYCLE_BIN; e->color = icon_colors[ICON_RECYCLE_BIN];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Control Panel"); strcpy(e->path, "/control");
        e->is_directory = 1; e->icon = ICON_CONTROL_PANEL; e->color = icon_colors[ICON_CONTROL_PANEL];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "ZEBX OS"); strcpy(e->path, "/zebx");
        e->is_directory = 1; e->icon = ICON_ZEBX_OS; e->color = icon_colors[ICON_ZEBX_OS];
    } else if (!strcmp(fm->current_path, "/home/Desktop")) {
        file_entry_t *e = &fm->items[fm->num_items++];
        strcpy(e->name, "This PC"); strcpy(e->path, "/");
        e->is_directory = 1; e->icon = ICON_COMPUTER; e->color = icon_colors[ICON_COMPUTER];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Recycle Bin"); strcpy(e->path, "/trash");
        e->is_directory = 1; e->icon = ICON_RECYCLE_BIN; e->color = icon_colors[ICON_RECYCLE_BIN];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "ZEBX Browser"); strcpy(e->path, "/apps/browser");
        e->is_directory = 0; e->icon = ICON_FILE_EXECUTABLE; e->color = icon_colors[ICON_FILE_EXECUTABLE];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Calculator"); strcpy(e->path, "/apps/calculator");
        e->is_directory = 0; e->icon = ICON_FILE_EXECUTABLE; e->color = icon_colors[ICON_FILE_EXECUTABLE];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "Terminal"); strcpy(e->path, "/apps/terminal");
        e->is_directory = 0; e->icon = ICON_FILE_EXECUTABLE; e->color = icon_colors[ICON_FILE_EXECUTABLE];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "welcome.txt"); strcpy(e->path, "/home/Desktop/welcome.txt");
        e->is_directory = 0; e->size = 1024; e->icon = ICON_FILE_TEXT; e->color = icon_colors[ICON_FILE_TEXT];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "zebx_wallpaper.png"); strcpy(e->path, "/home/Desktop/zebx_wallpaper.png");
        e->is_directory = 0; e->size = 2048576; e->icon = ICON_FILE_IMAGE; e->color = icon_colors[ICON_FILE_IMAGE];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "system.log"); strcpy(e->path, "/home/Desktop/system.log");
        e->is_directory = 0; e->size = 4096; e->icon = ICON_FILE_LOG; e->color = icon_colors[ICON_FILE_LOG];
    } else {
        file_entry_t *e = &fm->items[fm->num_items++];
        strcpy(e->name, ".."); strcpy(e->path, "..");
        e->is_directory = 1; e->icon = ICON_FOLDER_OPEN; e->color = icon_colors[ICON_FOLDER_OPEN];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "sample.txt"); strcpy(e->path, "/sample.txt");
        e->is_directory = 0; e->size = 512; e->icon = ICON_FILE_TEXT; e->color = icon_colors[ICON_FILE_TEXT];

        e = &fm->items[fm->num_items++];
        strcpy(e->name, "readme.md"); strcpy(e->path, "/readme.md");
        e->is_directory = 0; e->size = 2048; e->icon = ICON_FILE_DOCUMENT; e->color = icon_colors[ICON_FILE_DOCUMENT];
    }

    // Sort
    for (uint32_t i = 0; i < fm->num_items; i++) {
        for (uint32_t j = i + 1; j < fm->num_items; j++) {
            int swap = 0;
            switch (fm->sort_mode) {
                case FM_SORT_NAME: swap = strcmp(fm->items[i].name, fm->items[j].name) > 0; break;
                case FM_SORT_SIZE: swap = fm->items[i].size > fm->items[j].size; break;
                case FM_SORT_TYPE: swap = fm->items[i].icon > fm->items[j].icon; break;
                case FM_SORT_DATE: swap = fm->items[i].size > fm->items[j].size; break;
            }
            if (!fm->sort_ascending) swap = !swap;
            if (swap) { file_entry_t tmp = fm->items[i]; fm->items[i] = fm->items[j]; fm->items[j] = tmp; }
        }
    }
}

void filemanager_navigate(file_manager_t *fm, const char *path) {
    if (fm->history_count < 100) {
        strcpy(fm->history[fm->history_count++], fm->current_path);
        fm->history_pos = fm->history_count - 1;
    }
    strncpy(fm->current_path, path, FM_MAX_PATH - 1);
    filemanager_refresh(fm);
}

void filemanager_go_up(file_manager_t *fm) {
    char *last_slash = NULL;
    for (int i = 0; fm->current_path[i]; i++) {
        if (fm->current_path[i] == '/') last_slash = &fm->current_path[i];
    }
    if (last_slash && last_slash != fm->current_path) {
        *last_slash = 0;
        if (strlen(fm->current_path) == 0) strcpy(fm->current_path, "/");
        filemanager_navigate(fm, fm->current_path);
    }
}

void filemanager_go_back(file_manager_t *fm) {
    if (fm->history_pos > 0) {
        fm->history_pos--;
        strcpy(fm->current_path, fm->history[fm->history_pos]);
        filemanager_refresh(fm);
    }
}

void filemanager_go_forward(file_manager_t *fm) {
    if (fm->history_pos < fm->history_count - 1) {
        fm->history_pos++;
        strcpy(fm->current_path, fm->history[fm->history_pos]);
        filemanager_refresh(fm);
    }
}

void filemanager_go_home(file_manager_t *fm) { filemanager_navigate(fm, "/home"); }
void filemanager_go_desktop(file_manager_t *fm) { filemanager_navigate(fm, "/home/Desktop"); }

void filemanager_select(file_manager_t *fm, uint32_t index) {
    if (index >= fm->num_items) return;
    if (!fm->selected[index]) { fm->selected[index] = 1; fm->num_selected++; fm->selected_index = index; }
}

void filemanager_select_all(file_manager_t *fm) {
    for (uint32_t i = 0; i < fm->num_items; i++) fm->selected[i] = 1;
    fm->num_selected = fm->num_items;
}

void filemanager_deselect_all(file_manager_t *fm) {
    memset(fm->selected, 0, sizeof(fm->selected));
    fm->num_selected = 0;
}

void filemanager_set_view(file_manager_t *fm, fm_view_mode_t mode) { fm->view_mode = mode; }
void filemanager_set_sort(file_manager_t *fm, fm_sort_mode_t mode, int ascending) {
    fm->sort_mode = mode; fm->sort_ascending = ascending; filemanager_refresh(fm);
}

file_icon_t filemanager_detect_icon(const char *filename, int is_dir) {
    if (is_dir) return ICON_FOLDER;
    int len = strlen(filename);
    if (len < 4) return ICON_FILE_GENERIC;
    const char *ext = filename + len - 4;
    if (!strcmp(ext, ".txt") || !strcmp(ext, ".md")) return ICON_FILE_TEXT;
    if (!strcmp(ext, ".png") || !strcmp(ext, ".jpg") || !strcmp(ext, ".bmp")) return ICON_FILE_IMAGE;
    if (!strcmp(ext, ".mp4") || !strcmp(ext, ".avi")) return ICON_FILE_VIDEO;
    if (!strcmp(ext, ".mp3") || !strcmp(ext, ".wav")) return ICON_FILE_AUDIO;
    if (!strcmp(ext, ".c") || !strcmp(ext, ".cpp") || !strcmp(ext, ".h")) return ICON_FILE_CODE;
    if (!strcmp(ext, ".zip") || !strcmp(ext, ".tar")) return ICON_FILE_ARCHIVE;
    if (!strcmp(ext, ".exe") || !strcmp(ext, ".bin")) return ICON_FILE_EXECUTABLE;
    if (!strcmp(ext, ".doc") || !strcmp(ext, ".pdf")) return ICON_FILE_DOCUMENT;
    if (!strcmp(ext, ".xls") || !strcmp(ext, ".csv")) return ICON_FILE_SPREADSHEET;
    if (!strcmp(ext, ".ppt")) return ICON_FILE_PRESENTATION;
    if (!strcmp(ext, ".db") || !strcmp(ext, ".sql")) return ICON_FILE_DATABASE;
    if (!strcmp(ext, ".cfg") || !strcmp(ext, ".ini")) return ICON_FILE_CONFIG;
    if (!strcmp(ext, ".log")) return ICON_FILE_LOG;
    if (!strcmp(ext, ".ttf") || !strcmp(ext, ".otf")) return ICON_FILE_FONT;
    return ICON_FILE_GENERIC;
}

uint32_t filemanager_detect_color(file_icon_t icon) {
    if (icon < NUM_ICONS) return icon_colors[icon];
    return 0xFF90A4AE;

}

void filemanager_render(file_manager_t *fm, uint32_t *buffer, int w, int h) {
    fm->window_w = w; fm->window_h = h;
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFFF5F5F5;
    filemanager_render_sidebar(fm, buffer, w, h);
    filemanager_render_toolbar(fm, buffer, w, h);
    filemanager_render_statusbar(fm, buffer, w, h);
    filemanager_render_items(fm, buffer, w, h);
    if (fm->show_logo) filemanager_render_zebx_logo(w - 60, 5, 40, buffer, w, h);
    if (fm->show_logs) filemanager_render_zebx_logs(buffer, w - 250, h - 150, 240, 120);
}

void filemanager_render_sidebar(file_manager_t *fm, uint32_t *buffer, int w, int h) {
    int sidebar_y = fm->toolbar_h;
    int sidebar_h = h - fm->toolbar_h - fm->statusbar_h;
    for (int row = sidebar_y; row < sidebar_y + sidebar_h && row < h; row++) {
        for (int col = 0; col < fm->sidebar_w && col < w; col++) {
            buffer[row * w + col] = 0xFFE8E8E8;
        }
    }
    for (int row = sidebar_y; row < sidebar_y + sidebar_h && row < h; row++) {
        buffer[row * w + fm->sidebar_w] = 0xFFD0D0D0;
    }

    filemanager_render_zebx_logo(10, sidebar_y + 10, 32, buffer, w, h);

    int y = sidebar_y + 55;
    for (uint32_t i = 0; i < fm->num_bookmarks; i++) {
        int item_y = y + i * 30;
        if (i == fm->selected_index) {
            for (int row = item_y; row < item_y + 28 && row < h; row++) {
                for (int col = 5; col < fm->sidebar_w - 5 && col < w; col++) {
                    buffer[row * w + col] = 0xFFCCE8FF;
                }
            }
        }
        file_icon_t icon = ICON_FOLDER;
        if (!strcmp(fm->bookmark_names[i], "Desktop")) icon = ICON_DESKTOP;
        else if (!strcmp(fm->bookmark_names[i], "Documents")) icon = ICON_DOCUMENTS;
        else if (!strcmp(fm->bookmark_names[i], "Downloads")) icon = ICON_DOWNLOADS;
        else if (!strcmp(fm->bookmark_names[i], "Pictures")) icon = ICON_PICTURES;
        else if (!strcmp(fm->bookmark_names[i], "Music")) icon = ICON_MUSIC;
        else if (!strcmp(fm->bookmark_names[i], "Videos")) icon = ICON_VIDEOS;
        else if (!strcmp(fm->bookmark_names[i], "This PC")) icon = ICON_COMPUTER;
        filemanager_render_icon(icon, 15, item_y + 4, 20, buffer, w, h);
    }
}

void filemanager_render_toolbar(file_manager_t *fm, uint32_t *buffer, int w, int h) {
    for (int row = 0; row < fm->toolbar_h && row < h; row++) {
        for (int col = 0; col < w; col++) buffer[row * w + col] = 0xFFFFFFFF;
    }
    for (int col = 0; col < w; col++) {
        buffer[(fm->toolbar_h - 1) * w + col] = 0xFFD0D0D0;
    }
    int btn_y = 8, btn_size = 24;
    for (int row = btn_y; row < btn_y + btn_size && row < h; row++) {
        for (int col = 10; col < 10 + btn_size && col < w; col++) buffer[row * w + col] = 0xFFE0E0E0;
        for (int col = 40; col < 40 + btn_size && col < w; col++) buffer[row * w + col] = 0xFFE0E0E0;
        for (int col = 70; col < 70 + btn_size && col < w; col++) buffer[row * w + col] = 0xFFE0E0E0;
    }
    int addr_x = 110, addr_w = w - addr_x - 150;
    for (int row = 6; row < fm->toolbar_h - 6 && row < h; row++) {
        for (int col = addr_x; col < addr_x + addr_w && col < w; col++) buffer[row * w + col] = 0xFFF0F0F0;
    }
    for (int col = addr_x; col < addr_x + addr_w && col < w; col++) {
        buffer[6 * w + col] = 0xFFD0D0D0;
        buffer[(fm->toolbar_h - 7) * w + col] = 0xFFD0D0D0;
    }
    int search_x = addr_x + addr_w + 10;
    for (int row = 6; row < fm->toolbar_h - 6 && row < h; row++) {
        for (int col = search_x; col < search_x + 130 && col < w; col++) buffer[row * w + col] = 0xFFF0F0F0;
    }
}

void filemanager_render_statusbar(file_manager_t *fm, uint32_t *buffer, int w, int h) {
    int sb_y = h - fm->statusbar_h;
    for (int row = sb_y; row < h; row++) {
        for (int col = 0; col < w; col++) buffer[row * w + col] = 0xFFE8E8E8;
    }
    for (int col = 0; col < w; col++) buffer[sb_y * w + col] = 0xFFD0D0D0;
}

void filemanager_render_items(file_manager_t *fm, uint32_t *buffer, int w, int h) {
    int content_x = fm->sidebar_w + 10;
    int content_y = fm->toolbar_h + 10;
    int content_w = w - fm->sidebar_w - 20;
    int content_h = h - fm->toolbar_h - fm->statusbar_h - 20;

    if (fm->view_mode == FM_VIEW_ICONS) {
        int icon_w = 80, icon_h = 90;
        int cols = content_w / icon_w; if (cols < 1) cols = 1;
        for (uint32_t i = 0; i < fm->num_items; i++) {
            int col = i % cols, row = i / cols;
            int ix = content_x + col * icon_w;
            int iy = content_y + row * icon_h - fm->scroll_offset;
            if (iy + icon_h < content_y || iy > content_y + content_h) continue;
            if (fm->selected[i]) {
                for (int r = iy; r < iy + icon_h && r < h; r++) {
                    for (int c = ix; c < ix + icon_w && c < w; c++) buffer[r * w + c] = 0xFFCCE8FF;
                }
            }
            filemanager_render_icon(fm->items[i].icon, ix + 16, iy + 5, 48, buffer, w, h);
        }
    } else if (fm->view_mode == FM_VIEW_LIST) {
        int row_h = 25;
        for (uint32_t i = 0; i < fm->num_items; i++) {
            int iy = content_y + i * row_h - fm->scroll_offset;
            if (iy + row_h < content_y || iy > content_y + content_h) continue;
            if (fm->selected[i]) {
                for (int r = iy; r < iy + row_h && r < h; r++) {
                    for (int c = content_x; c < content_x + content_w && c < w; c++) buffer[r * w + c] = 0xFFCCE8FF;
                }
            }
            filemanager_render_icon(fm->items[i].icon, content_x + 5, iy + 2, 20, buffer, w, h);
            if (!fm->selected[i]) {
                for (int r = iy; r < iy + row_h && r < h; r++) {
                    for (int c = content_x; c < content_x + content_w && c < w; c++) {
                        if (i % 2 == 1) buffer[r * w + c] = 0xFFFAFAFA;
                    }
                }
            }
        }
    }
}

void filemanager_render_icon(file_icon_t icon, int x, int y, int size, uint32_t *buffer, int bw, int bh) {
    if (x < 0 || y < 0 || x + size > bw || y + size > bh) return;
    uint32_t color = (icon < NUM_ICONS) ? icon_colors[icon] : 0xFF90A4AE;

    if (icon == ICON_FOLDER || icon == ICON_FOLDER_OPEN) {
        for (int row = y + 8; row < y + size - 4 && row < bh; row++) {
            for (int col = x + 4; col < x + size - 4 && col < bw; col++) buffer[row * bw + col] = color;
        }
        for (int row = y + 2; row < y + 12 && row < bh; row++) {
            for (int col = x + 4; col < x + size / 2 && col < bw; col++) buffer[row * bw + col] = color;
        }
        for (int row = y + 8; row < y + 14 && row < bh; row++) {
            for (int col = x + 6; col < x + size - 6 && col < bw; col++) {
                uint32_t r = ((color >> 16) & 0xFF) + 30;
                uint32_t g = ((color >> 8) & 0xFF) + 30;
                uint32_t b = (color & 0xFF) + 30;
                if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
                buffer[row * bw + col] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    } else if (icon == ICON_DRIVE || icon == ICON_DRIVE_USB || icon == ICON_DRIVE_CD) {
        for (int row = y + 4; row < y + size - 8 && row < bh; row++) {
            for (int col = x + 4; col < x + size - 4 && col < bw; col++) buffer[row * bw + col] = color;
        }
        for (int row = y + size - 8; row < y + size - 4 && row < bh; row++) {
            for (int col = x + size / 3; col < x + 2 * size / 3 && col < bw; col++) buffer[row * bw + col] = 0xFF666666;
        }
        for (int row = y + size - 12; row < y + size - 8 && row < bh; row++) {
            for (int col = x + size - 12; col < x + size - 8 && col < bw; col++) buffer[row * bw + col] = 0xFF00FF00;
        }
    } else if (icon == ICON_FILE_GENERIC || icon == ICON_FILE_TEXT || icon == ICON_FILE_CODE) {
        for (int row = y + 4; row < y + size - 8 && row < bh; row++) {
            for (int col = x + 8; col < x + size - 8 && col < bw; col++) buffer[row * bw + col] = color;
        }
        for (int row = y + 4; row < y + 14 && row < bh; row++) {
            for (int col = x + size - 18; col < x + size - 8 && col < bw; col++) buffer[row * bw + col] = color;
        }
        for (int row = y + 4; row < y + 12 && row < bh; row++) {
            for (int col = x + size - 16; col < x + size - 10 && col < bw; col++) {
                uint32_t r = ((color >> 16) & 0xFF) + 20;
                uint32_t g = ((color >> 8) & 0xFF) + 20;
                uint32_t b = (color & 0xFF) + 20;
                if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
                buffer[row * bw + col] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
        for (int row = y + size - 16; row < y + size - 10 && row < bh; row++) {
            for (int col = x + 12; col < x + size - 12 && col < bw; col++) {
                uint32_t dark_r = ((color >> 16) & 0xFF) * 0.7;
                uint32_t dark_g = ((color >> 8) & 0xFF) * 0.7;
                uint32_t dark_b = (color & 0xFF) * 0.7;
                buffer[row * bw + col] = 0xFF000000 | (dark_r << 16) | (dark_g << 8) | dark_b;
            }
        }
    } else if (icon == ICON_ZEBX_LOGO || icon == ICON_ZEBX_OS) {
        filemanager_render_zebx_logo(x, y, size, buffer, bw, bh);
    } else {
        for (int row = y + 4; row < y + size - 4 && row < bh; row++) {
            for (int col = x + 4; col < x + size - 4 && col < bw; col++) buffer[row * bw + col] = color;
        }
    }
}

void filemanager_render_zebx_logo(int x, int y, int size, uint32_t *buffer, int bw, int bh) {
    if (x < 0 || y < 0 || x + size > bw || y + size > bh) return;
    uint32_t cyan = 0xFF00BCD4, dark_cyan = 0xFF0097A7, white = 0xFFFFFFFF;
    int cx = x + size / 2, cy = y + size / 2, radius = size / 2 - 2;
    for (int row = y; row < y + size && row < bh; row++) {
        for (int col = x; col < x + size && col < bw; col++) {
            int dx = col - cx, dy = row - cy;
            if (dx * dx + dy * dy <= radius * radius) {
                int local_x = col - x, local_y = row - y;
                int thickness = size / 6;
                int in_top = (local_y >= size / 5 && local_y < size / 5 + thickness);
                int in_diag = (local_x >= size / 5 && local_x < size - size / 5) &&
                              (local_y >= size / 5 && local_y < size - size / 5) &&
                              (local_x - size / 5 >= local_y - size / 5 - thickness &&
                               local_x - size / 5 <= local_y - size / 5 + thickness);
                int in_bottom = (local_y >= size - size / 5 - thickness && local_y < size - size / 5);
                if (in_top || in_diag || in_bottom) {
                    buffer[row * bw + col] = white;
                } else {
                    float grad = (float)(row - y) / size;
                    uint32_t r = (uint32_t)(((cyan >> 16) & 0xFF) * (1 - grad) + ((dark_cyan >> 16) & 0xFF) * grad);
                    uint32_t g = (uint32_t)(((cyan >> 8) & 0xFF) * (1 - grad) + ((dark_cyan >> 8) & 0xFF) * grad);
                    uint32_t b = (uint32_t)((cyan & 0xFF) * (1 - grad) + (dark_cyan & 0xFF) * grad);
                    buffer[row * bw + col] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }
    }
}

void filemanager_render_zebx_logs(uint32_t *buffer, int x, int y, int w, int h) {
    if (x < 0 || y < 0) return;
    for (int row = y; row < y + h && row < 600; row++) {
        for (int col = x; col < x + w && col < 800; col++) {
            buffer[row * 800 + col] = 0xCC1A1A2E;
        }
    }
    for (int col = x; col < x + w && col < 800; col++) {
        buffer[y * 800 + col] = 0xFF00BCD4;
        buffer[(y + h - 1) * 800 + col] = 0xFF00BCD4;
    }
    for (int row = y; row < y + h && row < 600; row++) {
        buffer[row * 800 + x] = 0xFF00BCD4;
        buffer[row * 800 + x + w - 1] = 0xFF00BCD4;
    }
}

void filemanager_show_system_info(file_manager_t *fm) {
    vga_puts("\\n--- System Information ---\\n");
    vga_puts("OS: ZEBX OS v1.0\\n");
    vga_puts("Kernel: x86_64\\n");
    vga_puts("Cores: 896 (P-cores + E-cores)\\n");
    vga_puts("SIMD: AVX-512F\\n");
    vga_puts("Storage: 3GB ZEBXFS\\n");
    vga_puts("Network: 1Gbps Gaming\\n");
    vga_puts("Display: 1920x1080 HDR\\n");
    vga_puts("Browser: ZBX-WebKit\\n");
}

void filemanager_show_zebx_about(file_manager_t *fm) {
    vga_puts("\\n--- About ZEBX OS ---\\n");
    vga_puts("\\nFeatures:\\n");
    vga_puts("  - AVX-512 SIMD Engine\\n");
    vga_puts("  - Intel Architecture Support\\n");
    vga_puts("  - HDR / VRR Display\\n");
    vga_puts("  - Web Browser Engine\\n");
    vga_puts("  - best calculator\\n");
    vga_puts("  - applications\\n");
    vga_puts("  - 1Gbps  Network\\n");
    vga_puts("\\n(c) 2026 ZEBX Corporation\\n");
}

void filemanager_add_bookmark(file_manager_t *fm, const char *path, const char *name) {
    if (fm->num_bookmarks >= FM_MAX_BOOKMARKS) return;
    strncpy(fm->bookmarks[fm->num_bookmarks], path, FM_MAX_PATH - 1);
    strncpy(fm->bookmark_names[fm->num_bookmarks], name, 63);
    fm->num_bookmarks++;
}

void filemanager_remove_bookmark(file_manager_t *fm, uint32_t index) {
    if (index >= fm->num_bookmarks) return;
    for (uint32_t i = index; i < fm->num_bookmarks - 1; i++) {
        strcpy(fm->bookmarks[i], fm->bookmarks[i + 1]);
        strcpy(fm->bookmark_names[i], fm->bookmark_names[i + 1]);
    }
    fm->num_bookmarks--;
}
