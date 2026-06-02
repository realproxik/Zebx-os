// ============================================================================
 // ZEBX OS - APP STORE INTERFACE by jatin labs
 // ============================================================================
#ifndef STORE_H
#define STORE_H

#include "types.h"

#define STORE_MAX_APPS 256
#define STORE_MAX_CATEGORIES 16
#define STORE_MAX_REVIEWS 100
#define STORE_NAME_LEN 64
#define STORE_DESC_LEN 512
#define STORE_DEV_LEN 64
#define STORE_VERSION_LEN 16
#define STORE_ICON_SIZE 64

// App categories
typedef enum {
    STORE_CAT_GAMES,
    STORE_CAT_PRODUCTIVITY,
    STORE_CAT_ENTERTAINMENT,
    STORE_CAT_SOCIAL,
    STORE_CAT_EDUCATION,
    STORE_CAT_DEVELOPER,
    STORE_CAT_UTILITIES,
    STORE_CAT_MULTIMEDIA,
    STORE_CAT_SECURITY,
    STORE_CAT_SYSTEM,
    STORE_CAT_BUSINESS,
    STORE_CAT_HEALTH,
    STORE_CAT_NEWS,
    STORE_CAT_TRAVEL,
    STORE_CAT_PHOTO,
    STORE_CAT_MUSIC,
} store_category_t;

// App status
typedef enum {
    STORE_STATUS_AVAILABLE,
    STORE_STATUS_INSTALLED,
    STORE_STATUS_UPDATING,
    STORE_STATUS_DOWNLOADING,
    STORE_STATUS_PENDING,
} store_status_t;

// Review
typedef struct {
    char user[32];
    uint8_t rating; // 1-5
    char text[256];
    uint64_t date;
} store_review_t;

// App entry
typedef struct {
    uint32_t id;
    char name[STORE_NAME_LEN];
    char description[STORE_DESC_LEN];
    char developer[STORE_DEV_LEN];
    char version[STORE_VERSION_LEN];
    store_category_t category;
    store_status_t status;
    uint32_t size_kb;
    uint32_t download_progress;
    uint32_t rating; // Average rating * 10 (e.g., 45 = 4.5)
    uint32_t num_downloads;
    uint32_t num_reviews;
    store_review_t reviews[STORE_MAX_REVIEWS];
    char icon_path[128];
    char binary_path[128];
    char screenshots[4][128];
    int free;
    int featured;
    int editor_pick;
    uint32_t color;
} store_app_t;

// Store state
typedef struct {
    store_app_t apps[STORE_MAX_APPS];
    uint32_t num_apps;
    store_category_t current_category;
    char search_query[128];
    int search_active;
    uint32_t featured_apps[8];
    uint32_t num_featured;
} store_state_t;

void store_init(void);
store_state_t* store_get_state(void);

// App management
void store_add_app(store_app_t *app);
void store_remove_app(uint32_t id);
store_app_t* store_get_app(uint32_t id);
store_app_t* store_find_app(const char *name);
store_app_t* store_get_apps_by_category(store_category_t cat, uint32_t *count);
store_app_t* store_search(const char *query, uint32_t *count);
store_app_t* store_get_featured(uint32_t *count);
store_app_t* store_get_free(uint32_t *count);
store_app_t* store_get_top_rated(uint32_t *count);
store_app_t* store_get_most_downloaded(uint32_t *count);

// Install / Update / Uninstall
int store_install(uint32_t app_id);
int store_update(uint32_t app_id);
int store_uninstall(uint32_t app_id);
int store_download(uint32_t app_id);
void store_cancel_download(uint32_t app_id);

// Reviews
void store_add_review(uint32_t app_id, store_review_t *review);
store_review_t* store_get_reviews(uint32_t app_id, uint32_t *count);

// Categories
const char* store_category_name(store_category_t cat);

// Rendering
void store_render_main(store_state_t *store, uint32_t *buffer, int w, int h);
void store_render_category(store_state_t *store, store_category_t cat, uint32_t *buffer, int w, int h);
void store_render_app_detail(store_app_t *app, uint32_t *buffer, int w, int h);
void store_render_search(store_state_t *store, uint32_t *buffer, int w, int h);
void store_render_downloads(store_state_t *store, uint32_t *buffer, int w, int h);
void store_render_library(store_state_t *store, uint32_t *buffer, int w, int h);
void store_render_app_tile(store_app_t *app, int x, int y, int w, int h, uint32_t *buffer, int bw, int bh);
void store_render_star_rating(uint32_t rating, int x, int y, uint32_t *buffer, int bw, int bh);

#endif

#include "store.h"
#include "vga.h"
#include "memory.h"

static store_state_t store_state;

// Category names
static const char *category_names[] = {
    "Games", "Productivity", "Entertainment", "Social", "Education",
    "Developer", "Utilities", "Multimedia", "Security", "System",
    "Business", "Health", "News", "Travel", "Photo", "Music"
};

// Category colors
static uint32_t category_colors[] = {
    0xFFFF6B6B, 0xFF4ECDC4, 0xFFFFE66D, 0xFF95E1D3, 0xFFA8E6CF,
    0xFFFD79A8, 0xFF74B9FF, 0xFFFF7675, 0xFF00B894, 0xFF0984E3,
    0xFF6C5CE7, 0xFF00CEC9, 0xFFE17055, 0xFF81ECEC, 0xFFFDCB6E, 0xFFDDA0DD
};

// Sample apps data
static store_app_t sample_apps[] = {
    {
        .id = 1,
        .name = "ZEBX Browser",
        .description = "Fast and secure web browser with HTML5, CSS3, and JavaScript support. Built-in ad blocker and privacy mode.",
        .developer = "ZEBX Corp",
        .version = "1.0.0",
        .category = STORE_CAT_PRODUCTIVITY,
        .status = STORE_STATUS_INSTALLED,
        .size_kb = 15360,
        .rating = 48,
        .num_downloads = 5000000,
        .num_reviews = 12500,
        .free = 1,
        .featured = 1,
        .editor_pick = 1,
        .color = 0xFF4285F4
    },
    {
        .id = 2,
        .name = "ZEBX Calculator",
        .description = "Scientific calculator with graphing, programmer mode, and unit converter. Supports complex numbers and matrices.",
        .developer = "ZEBX Corp",
        .version = "2.1.0",
        .category = STORE_CAT_PRODUCTIVITY,
        .status = STORE_STATUS_INSTALLED,
        .size_kb = 8192,
        .rating = 47,
        .num_downloads = 3200000,
        .num_reviews = 8900,
        .free = 1,
        .featured = 1,
        .editor_pick = 0,
        .color = 0xFFFF9500
    },
    {
        .id = 3,
        .name = "ZEBX Terminal",
        .description = "Powerful terminal emulator with multi-tab support, custom themes, and full ZEBX OS command set.",
        .developer = "ZEBX Corp",
        .version = "1.5.0",
        .category = STORE_CAT_DEVELOPER,
        .status = STORE_STATUS_INSTALLED,
        .size_kb = 6144,
        .rating = 49,
        .num_downloads = 2800000,
        .num_reviews = 7200,
        .free = 1,
        .featured = 1,
        .editor_pick = 1,
        .color = 0xFF34C759
    },
    {
        .id = 4,
        .name = "ZEBX Code Editor",
        .description = "Professional code editor with syntax highlighting for C, C++, Python, C#, and more. IntelliSense and debugging.",
        .developer = "ZEBX Corp",
        .version = "3.0.0",
        .category = STORE_CAT_DEVELOPER,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 24576,
        .rating = 46,
        .num_downloads = 1500000,
        .num_reviews = 5600,
        .free = 1,
        .featured = 1,
        .editor_pick = 1,
        .color = 0xFF007ACC
    },
    {
        .id = 5,
        .name = "ZEBX Media Player",
        .description = "Universal media player supporting MP4, MKV, AVI, MP3, FLAC, and more. Hardware-accelerated decoding.",
        .developer = "ZEBX Corp",
        .version = "2.0.0",
        .category = STORE_CAT_MULTIMEDIA,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 18432,
        .rating = 45,
        .num_downloads = 4100000,
        .num_reviews = 9800,
        .free = 1,
        .featured = 1,
        .editor_pick = 0,
        .color = 0xFFFF3B30
    },
    {
        .id = 6,
        .name = "ZEBX File Manager",
        .description = "Advanced file manager with dual pane, cloud sync, and powerful search. Supports all archive formats.",
        .developer = "ZEBX Corp",
        .version = "1.8.0",
        .category = STORE_CAT_UTILITIES,
        .status = STORE_STATUS_INSTALLED,
        .size_kb = 12288,
        .rating = 46,
        .num_downloads = 3600000,
        .num_reviews = 8400,
        .free = 1,
        .featured = 0,
        .editor_pick = 1,
        .color = 0xFF5856D6
    },
    {
        .id = 7,
        .name = "ZEBX Game Engine",
        .description = "2D/3D game engine with physics, particle systems, and shader support. Build games in C++ or C#.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 51200,
        .rating = 47,
        .num_downloads = 890000,
        .num_reviews = 3200,
        .free = 1,
        .featured = 1,
        .editor_pick = 1,
        .color = 0xFFFF2D55
    },
    {
        .id = 8,
        .name = "ZEBX Paint",
        .description = "Digital painting and image editing tool with layers, brushes, and filters. PSD compatible.",
        .developer = "ZEBX Corp",
        .version = "1.2.0",
        .category = STORE_CAT_PHOTO,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 28672,
        .rating = 44,
        .num_downloads = 1200000,
        .num_reviews = 4500,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFFFF9500
    },
    {
        .id = 9,
        .name = "ZEBX Music",
        .description = "Music player and library manager with playlist support, equalizer, and visualizer.",
        .developer = "ZEBX Corp",
        .version = "1.0.0",
        .category = STORE_CAT_MUSIC,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 16384,
        .rating = 45,
        .num_downloads = 2100000,
        .num_reviews = 6700,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFFAF52DE
    },
    {
        .id = 10,
        .name = "ZEBX Defender",
        .description = "Antivirus and firewall protection. Real-time scanning, threat detection, and network monitoring.",
        .developer = "ZEBX Security",
        .version = "2.0.0",
        .category = STORE_CAT_SECURITY,
        .status = STORE_STATUS_INSTALLED,
        .size_kb = 20480,
        .rating = 48,
        .num_downloads = 4500000,
        .num_reviews = 11200,
        .free = 1,
        .featured = 1,
        .editor_pick = 1,
        .color = 0xFF34C759
    },
    {
        .id = 11,
        .name = "ZEBX Chat",
        .description = "Instant messaging app with end-to-end encryption, group chats, and file sharing.",
        .developer = "ZEBX Corp",
        .version = "1.0.0",
        .category = STORE_CAT_SOCIAL,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 14336,
        .rating = 43,
        .num_downloads = 1800000,
        .num_reviews = 5200,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFF007AFF
    },
    {
        .id = 12,
        .name = "ZEBX Mail",
        .description = "Email client supporting IMAP, POP3, and SMTP. Multiple account support and spam filtering.",
        .developer = "ZEBX Corp",
        .version = "1.0.0",
        .category = STORE_CAT_PRODUCTIVITY,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 10240,
        .rating = 44,
        .num_downloads = 950000,
        .num_reviews = 3100,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFF5AC8FA
    },
    // Games
    {
        .id = 13,
        .name = "ZEBX Racer",
        .description = "High-speed racing game with realistic physics, multiple tracks, and multiplayer support.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 102400,
        .rating = 47,
        .num_downloads = 750000,
        .num_reviews = 2800,
        .free = 1,
        .featured = 1,
        .editor_pick = 0,
        .color = 0xFFFF3B30
    },
    {
        .id = 14,
        .name = "ZEBX Shooter",
        .description = "First-person shooter with campaign mode, multiplayer battles, and weapon customization.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 153600,
        .rating = 46,
        .num_downloads = 620000,
        .num_reviews = 2400,
        .free = 1,
        .featured = 1,
        .editor_pick = 0,
        .color = 0xFFFF2D55
    },
    {
        .id = 15,
        .name = "ZEBX Puzzle",
        .description = "Mind-bending puzzle game with 100+ levels. Challenge your brain with logic puzzles.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 8192,
        .rating = 45,
        .num_downloads = 890000,
        .num_reviews = 3600,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFF5856D6
    },
    {
        .id = 16,
        .name = "ZEBX Chess",
        .description = "Classic chess with AI opponent, online multiplayer, and puzzle mode. ELO rating system.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 12288,
        .rating = 48,
        .num_downloads = 1100000,
        .num_reviews = 4200,
        .free = 1,
        .featured = 0,
        .editor_pick = 1,
        .color = 0xFF8E8E93
    },
    {
        .id = 17,
        .name = "ZEBX RPG",
        .description = "Epic role-playing game with open world, character progression, and quest system.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 204800,
        .rating = 49,
        .num_downloads = 450000,
        .num_reviews = 1800,
        .free = 1,
        .featured = 1,
        .editor_pick = 1,
        .color = 0xFFFF9500
    },
    {
        .id = 18,
        .name = "ZEBX Strategy",
        .description = "Real-time strategy game with base building, resource management, and tactical combat.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 81920,
        .rating = 46,
        .num_downloads = 380000,
        .num_reviews = 1500,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFF34C759
    },
    {
        .id = 19,
        .name = "ZEBX Platformer",
        .description = "Classic 2D platformer with pixel art, power-ups, and boss battles.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 16384,
        .rating = 44,
        .num_downloads = 520000,
        .num_reviews = 2100,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFFFF6B6B
    },
    {
        .id = 20,
        .name = "ZEBX Simulator",
        .description = "Life simulation game with crafting, farming, and town building mechanics.",
        .developer = "ZEBX Games",
        .version = "1.0.0",
        .category = STORE_CAT_GAMES,
        .status = STORE_STATUS_AVAILABLE,
        .size_kb = 40960,
        .rating = 45,
        .num_downloads = 680000,
        .num_reviews = 2600,
        .free = 1,
        .featured = 0,
        .editor_pick = 0,
        .color = 0xFF4ECDC4
    },
};

#define NUM_SAMPLE_APPS (sizeof(sample_apps) / sizeof(sample_apps[0]))

void store_init(void) {
    memset(&store_state, 0, sizeof(store_state));
    store_state.current_category = STORE_CAT_GAMES;
    
    // Load sample apps
    for (uint32_t i = 0; i < NUM_SAMPLE_APPS && i < STORE_MAX_APPS; i++) {
        store_state.apps[store_state.num_apps++] = sample_apps[i];
    }
    
    // Set featured apps
    uint32_t fidx = 0;
    for (uint32_t i = 0; i < store_state.num_apps && fidx < 8; i++) {
        if (store_state.apps[i].featured) {
            store_state.featured_apps[fidx++] = i;
        }
    }
    store_state.num_featured = fidx;
    
    vga_puts("[+] ZEBX Store initialized\\n");
    vga_puts("    Apps: ");
    vga_put_int(store_state.num_apps);
    vga_puts("\\n");
    vga_puts("    Categories: ");
    vga_put_int(STORE_MAX_CATEGORIES);
    vga_puts("\\n");
    vga_puts("    All apps are FREE!\\n");
}

store_state_t* store_get_state(void) {
    return &store_state;
}

void store_add_app(store_app_t *app) {
    if (store_state.num_apps >= STORE_MAX_APPS) return;
    store_state.apps[store_state.num_apps++] = *app;
}

void store_remove_app(uint32_t id) {
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        if (store_state.apps[i].id == id) {
            for (uint32_t j = i; j < store_state.num_apps - 1; j++) {
                store_state.apps[j] = store_state.apps[j + 1];
            }
            store_state.num_apps--;
            return;
        }
    }
}

store_app_t* store_get_app(uint32_t id) {
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        if (store_state.apps[i].id == id) return &store_state.apps[i];
    }
    return NULL;
}

store_app_t* store_find_app(const char *name) {
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        if (!strcmp(store_state.apps[i].name, name)) return &store_state.apps[i];
    }
    return NULL;
}

store_app_t* store_get_apps_by_category(store_category_t cat, uint32_t *count) {
    static store_app_t result[STORE_MAX_APPS];
    uint32_t n = 0;
    
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        if (store_state.apps[i].category == cat) {
            result[n++] = store_state.apps[i];
        }
    }
    
    *count = n;
    return result;
}

store_app_t* store_search(const char *query, uint32_t *count) {
    static store_app_t result[STORE_MAX_APPS];
    uint32_t n = 0;
    
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        if (strstr(store_state.apps[i].name, query) ||
            strstr(store_state.apps[i].description, query) ||
            strstr(store_state.apps[i].developer, query)) {
            result[n++] = store_state.apps[i];
        }
    }
    
    *count = n;
    return result;
}

store_app_t* store_get_featured(uint32_t *count) {
    static store_app_t result[8];
    uint32_t n = 0;
    
    for (uint32_t i = 0; i < store_state.num_featured; i++) {
        result[n++] = store_state.apps[store_state.featured_apps[i]];
    }
    
    *count = n;
    return result;
}

store_app_t* store_get_free(uint32_t *count) {
    static store_app_t result[STORE_MAX_APPS];
    uint32_t n = 0;
    
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        if (store_state.apps[i].free) {
            result[n++] = store_state.apps[i];
        }
    }
    
    *count = n;
    return result;
}

store_app_t* store_get_top_rated(uint32_t *count) {
    static store_app_t result[STORE_MAX_APPS];
    
    // Copy all apps
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        result[i] = store_state.apps[i];
    }
    
    // Sort by rating (bubble sort)
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        for (uint32_t j = i + 1; j < store_state.num_apps; j++) {
            if (result[j].rating > result[i].rating) {
                store_app_t tmp = result[i];
                result[i] = result[j];
                result[j] = tmp;
            }
        }
    }
    
    *count = store_state.num_apps;
    return result;
}

store_app_t* store_get_most_downloaded(uint32_t *count) {
    static store_app_t result[STORE_MAX_APPS];
    
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        result[i] = store_state.apps[i];
    }
    
    for (uint32_t i = 0; i < store_state.num_apps; i++) {
        for (uint32_t j = i + 1; j < store_state.num_apps; j++) {
            if (result[j].num_downloads > result[i].num_downloads) {
                store_app_t tmp = result[i];
                result[i] = result[j];
                result[j] = tmp;
            }
        }
    }
    
    *count = store_state.num_apps;
    return result;
}

int store_install(uint32_t app_id) {
    store_app_t *app = store_get_app(app_id);
    if (!app) return -1;
    if (app->status == STORE_STATUS_INSTALLED) return 0;
    
    app->status = STORE_STATUS_DOWNLOADING;
    app->download_progress = 0;
    
    vga_puts("[*] Installing: ");
    vga_puts(app->name);
    vga_puts("\\n");
    
    // Simulate download
    for (int i = 0; i <= 100; i += 10) {
        app->download_progress = i;
        for (volatile int j = 0; j < 100000; j++);
    }
    
    app->status = STORE_STATUS_INSTALLED;
    app->download_progress = 100;
    app->num_downloads++;
    
    vga_puts("[+] Installed: ");
    vga_puts(app->name);
    vga_puts("\\n");
    
    return 0;
}

int store_update(uint32_t app_id) {
    store_app_t *app = store_get_app(app_id);
    if (!app) return -1;
    
    vga_puts("[*] Updating: ");
    vga_puts(app->name);
    vga_puts("\\n");
    
    app->status = STORE_STATUS_UPDATING;
    
    // Simulate update
    for (volatile int i = 0; i < 500000; i++);
    
    app->status = STORE_STATUS_INSTALLED;
    
    vga_puts("[+] Updated: ");
    vga_puts(app->name);
    vga_puts("\\n");
    
    return 0;
}

int store_uninstall(uint32_t app_id) {
    store_app_t *app = store_get_app(app_id);
    if (!app) return -1;
    if (app->status != STORE_STATUS_INSTALLED) return -1;
    
    vga_puts("[*] Uninstalling: ");
    vga_puts(app->name);
    vga_puts("\\n");
    
    app->status = STORE_STATUS_AVAILABLE;
    
    vga_puts("[+] Uninstalled: ");
    vga_puts(app->name);
    vga_puts("\\n");
    
    return 0;
}

int store_download(uint32_t app_id) {
    return store_install(app_id);
}

void store_cancel_download(uint32_t app_id) {
    store_app_t *app = store_get_app(app_id);
    if (app && app->status == STORE_STATUS_DOWNLOADING) {
        app->status = STORE_STATUS_AVAILABLE;
        app->download_progress = 0;
    }
}

void store_add_review(uint32_t app_id, store_review_t *review) {
    store_app_t *app = store_get_app(app_id);
    if (!app || app->num_reviews >= STORE_MAX_REVIEWS) return;
    
    app->reviews[app->num_reviews++] = *review;
    
    // Recalculate average rating
    uint32_t total = 0;
    for (uint32_t i = 0; i < app->num_reviews; i++) {
        total += app->reviews[i].rating;
    }
    app->rating = (total * 10) / app->num_reviews;
}

store_review_t* store_get_reviews(uint32_t app_id, uint32_t *count) {
    store_app_t *app = store_get_app(app_id);
    if (!app) {
        *count = 0;
        return NULL;
    }
    
    *count = app->num_reviews;
    return app->reviews;
}

const char* store_category_name(store_category_t cat) {
    if (cat < STORE_MAX_CATEGORIES) return category_names[cat];
    return "Unknown";
}

// Rendering
void store_render_main(store_state_t *store, uint32_t *buffer, int w, int h) {
    // Background
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    // Header
    for (int row = 0; row < 60 && row < h; row++) {
        for (int col = 0; col < w; col++) {
            buffer[row * w + col] = 0xFF2D2D2D;
        }
    }
    
    // ZEBX Store logo
    for (int row = 10; row < 50 && row < h; row++) {
        for (int col = 20; col < 200 && col < w; col++) {
            buffer[row * w + col] = 0xFF00BCD4;
        }
    }
    
    // Search bar
    for (int row = 15; row < 45 && row < h; row++) {
        for (int col = 250; col < w - 100 && col < w; col++) {
            buffer[row * w + col] = 0xFF3D3D3D;
        }
    }
    
    // Featured section
    int section_y = 80;
    store_app_t *featured = store_get_featured(&store->num_featured);
    
    for (uint32_t i = 0; i < store->num_featured && i < 4; i++) {
        int fx = 20 + (i % 2) * (w / 2 - 10);
        int fy = section_y + (i / 2) * 120;
        store_render_app_tile(&featured[i], fx, fy, w / 2 - 20, 100, buffer, w, h);
    }
    
    // Categories
    section_y = 340;
    for (int cat = 0; cat < 8 && cat < STORE_MAX_CATEGORIES; cat++) {
        int cx = 20 + (cat % 4) * ((w - 40) / 4);
        int cy = section_y + (cat / 4) * 80;
        int cw = (w - 40) / 4 - 10;
        int ch = 60;
        
        for (int row = cy; row < cy + ch && row < h; row++) {
            for (int col = cx; col < cx + cw && col < w; col++) {
                buffer[row * w + col] = category_colors[cat];
            }
        }
    }
}

void store_render_app_tile(store_app_t *app, int x, int y, int tw, int th, uint32_t *buffer, int bw, int bh) {
    if (x < 0 || y < 0 || x + tw > bw || y + th > bh) return;
    
    // Background
    for (int row = y; row < y + th && row < bh; row++) {
        for (int col = x; col < x + tw && col < bw; col++) {
            buffer[row * bw + col] = 0xFF2D2D2D;
        }
    }
    
    // Icon area
    for (int row = y + 5; row < y + th - 5 && row < bh; row++) {
        for (int col = x + 5; col < x + tw / 3 && col < bw; col++) {
            buffer[row * bw + col] = app->color;
        }
    }
    
    // Rating stars
    store_render_star_rating(app->rating, x + tw / 3 + 10, y + th - 25, buffer, bw, bh);
    
    // Free badge
    if (app->free) {
        for (int row = y + 5; row < y + 25 && row < bh; row++) {
            for (int col = x + tw - 50; col < x + tw - 5 && col < bw; col++) {
                buffer[row * bw + col] = 0xFF00C853;
            }
        }
    }
}

void store_render_star_rating(uint32_t rating, int x, int y, uint32_t *buffer, int bw, int bh) {
    // rating is *10 (e.g., 45 = 4.5 stars)
    int full_stars = rating / 10;
    int half_star = (rating % 10) >= 5;
    
    for (int i = 0; i < 5; i++) {
        int sx = x + i * 15;
        uint32_t color = (i < full_stars) ? 0xFFFFD700 :
                        (i == full_stars && half_star) ? 0xFFFFA500 : 0xFF444444;
        
        for (int row = y; row < y + 12 && row < bh; row++) {
            for (int col = sx; col < sx + 12 && col < bw; col++) {
                buffer[row * bw + col] = color;
            }
        }
    }
}

void store_render_category(store_state_t *store, store_category_t cat, uint32_t *buffer, int w, int h) {
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    uint32_t count;
    store_app_t *apps = store_get_apps_by_category(cat, &count);
    
    int tile_w = (w - 60) / 3;
    int tile_h = 120;
    
    for (uint32_t i = 0; i < count && i < 12; i++) {
        int tx = 20 + (i % 3) * (tile_w + 10);
        int ty = 80 + (i / 3) * (tile_h + 10);
        store_render_app_tile(&apps[i], tx, ty, tile_w, tile_h, buffer, w, h);
    }
}

void store_render_app_detail(store_app_t *app, uint32_t *buffer, int w, int h) {
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    // Large icon
    for (int row = 40; row < 200 && row < h; row++) {
        for (int col = 40; col < 200 && col < w; col++) {
            buffer[row * w + col] = app->color;
        }
    }
    
    // Info area
    // Rating
    store_render_star_rating(app->rating, 220, 60, buffer, w, h);
    
    // Download button
    for (int row = h - 80; row < h - 30 && row < h; row++) {
        for (int col = w / 2 - 100; col < w / 2 + 100 && col < w; col++) {
            buffer[row * w + col] = (app->status == STORE_STATUS_INSTALLED) ? 0xFF34C759 : 0xFF007AFF;
        }
    }
}

void store_render_search(store_state_t *store, uint32_t *buffer, int w, int h) {
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    if (!store->search_active) return;
    
    uint32_t count;
    store_app_t *results = store_search(store->search_query, &count);
    
    int tile_w = w - 40;
    int tile_h = 80;
    
    for (uint32_t i = 0; i < count && i < 10; i++) {
        int ty = 80 + i * (tile_h + 10);
        store_render_app_tile(&results[i], 20, ty, tile_w, tile_h, buffer, w, h);
    }
}

void store_render_downloads(store_state_t *store, uint32_t *buffer, int w, int h) {
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    int y = 80;
    for (uint32_t i = 0; i < store->num_apps; i++) {
        if (store->apps[i].status == STORE_STATUS_DOWNLOADING ||
            store->apps[i].status == STORE_STATUS_UPDATING) {
            store_render_app_tile(&store->apps[i], 20, y, w - 40, 80, buffer, w, h);
            
            // Progress bar
            int pw = (w - 60) * store->apps[i].download_progress / 100;
            for (int row = y + 60; row < y + 70 && row < h; row++) {
                for (int col = 20; col < 20 + pw && col < w; col++) {
                    buffer[row * w + col] = 0xFF007AFF;
                }
            }
            
            y += 90;
        }
    }
}

void store_render_library(store_state_t *store, uint32_t *buffer, int w, int h) {
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    int y = 80;
    for (uint32_t i = 0; i < store->num_apps; i++) {
        if (store->apps[i].status == STORE_STATUS_INSTALLED) {
            store_render_app_tile(&store->apps[i], 20, y, w - 40, 80, buffer, w, h);
            y += 90;
        }
    }
}