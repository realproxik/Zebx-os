
#ifndef WALLPAPER_H
#define WALLPAPER_H

#include "types.h"

#define WP_MAX_LAYERS 16
#define WP_MAX_PARTICLES 1024
#define WP_MAX_ANIMATIONS 32

typedef enum {
    WP_TYPE_SOLID,
    WP_TYPE_GRADIENT,
    WP_TYPE_IMAGE,
    WP_TYPE_SLIDESHOW,
    WP_TYPE_LIVE,
    WP_TYPE_VIDEO,
    WP_TYPE_PARTICLE,
    WP_TYPE_INTERACTIVE,
} wp_type_t;

typedef struct {
    float x, y;
    float vx, vy;
    float size;
    uint32_t color;
    float life;
    float max_life;
} wp_particle_t;

typedef struct {
    char name[64];
    wp_type_t type;
    uint32_t color1;
    uint32_t color2;
    uint32_t color3;
    int gradient_direction; // 0=vertical, 1=horizontal, 2=radial, 3=diagonal
    char image_path[256];
    char xml_config[512];
    int animated;
    int particle_count;
    wp_particle_t particles[WP_MAX_PARTICLES];
    float animation_speed;
    int parallax_enabled;
    float parallax_strength;
} wallpaper_t;

void wallpaper_init(void);
void wallpaper_load_from_xml(const char *xml_path);
void wallpaper_render(uint32_t *buffer, int w, int h);
void wallpaper_update(float dt);
void wallpaper_set_type(wp_type_t type);
void wallpaper_set_colors(uint32_t c1, uint32_t c2, uint32_t c3);
void wallpaper_set_image(const char *path);
void wallpaper_set_animated(int animated);
wallpaper_t* wallpaper_get_current(void);

#endif
'''

with open(output_dir + '/kernel/wallpaper.h', 'w') as f:
    f.write(wallpaper_h)

wallpaper_c = '''// ============================================================================
// ZEBX OS - WALLPAPER ENGINE IMPLEMENTATION
// ============================================================================

#include "wallpaper.h"
#include "vga.h"
#include "memory.h"
#include "simd.h"

static wallpaper_t current_wallpaper;
static uint32_t *wallpaper_buffer = NULL;
static int wp_width = 1920, wp_height = 1080;

void wallpaper_init(void) {
    memset(&current_wallpaper, 0, sizeof(wallpaper_t));
    strcpy(current_wallpaper.name, "ZEBX Default");
    current_wallpaper.type = WP_TYPE_GRADIENT;
    current_wallpaper.color1 = 0xFF1A1A2E; // Dark blue
    current_wallpaper.color2 = 0xFF16213E; // Darker blue
    current_wallpaper.color3 = 0xFF0F3460; // Navy
    current_wallpaper.gradient_direction = 0; // Vertical
    current_wallpaper.animated = 1;
    current_wallpaper.animation_speed = 0.5f;
    current_wallpaper.parallax_enabled = 1;
    current_wallpaper.parallax_strength = 0.1f;
    current_wallpaper.particle_count = 64;

    // Initialize particles
    for (int i = 0; i < current_wallpaper.particle_count; i++) {
        wp_particle_t *p = &current_wallpaper.particles[i];
        p->x = (float)(i * 30) % wp_width;
        p->y = (float)(i * 17) % wp_height;
        p->vx = ((float)(i % 5) - 2.0f) * 0.5f;
        p->vy = ((float)(i % 3) - 1.0f) * 0.3f;
        p->size = 2.0f + (i % 4);
        p->color = 0xFF00BCD4;
        p->life = (float)i / current_wallpaper.particle_count;
        p->max_life = 1.0f;
    }

    wallpaper_buffer = (uint32_t*)kmalloc(wp_width * wp_height * 4);
    vga_puts("[+] Wallpaper Engine initialized\\n");
}

void wallpaper_load_from_xml(const char *xml_path) {
    // Parse XML wallpaper config
    // Format:
    // <wallpaper>
    //   <type>gradient</type>
    //   <color1>#1A1A2E</color1>
    //   <color2>#16213E</color2>
    //   <color3>#0F3460</color3>
    //   <direction>vertical</direction>
    //   <animated>true</animated>
    //   <particles>64</particles>
    //   <parallax>true</parallax>
    // </wallpaper>

    // Simplified XML parsing
    vga_puts("[*] Loading wallpaper from: ");
    vga_puts(xml_path);
    vga_puts("\\n");

    // For demo, apply default ZEBX theme
    current_wallpaper.type = WP_TYPE_GRADIENT;
    current_wallpaper.color1 = 0xFF1A1A2E;
    current_wallpaper.color2 = 0xFF16213E;
    current_wallpaper.color3 = 0xFF0F3460;
    current_wallpaper.gradient_direction = 0;
    current_wallpaper.animated = 1;
    current_wallpaper.particle_count = 64;
    current_wallpaper.parallax_enabled = 1;

    vga_puts("[+] Wallpaper loaded: ZEBX Dark Theme\\n");
}

void wallpaper_render(uint32_t *buffer, int w, int h) {
    if (current_wallpaper.type == WP_TYPE_GRADIENT) {
        // Vertical gradient
        for (int row = 0; row < h; row++) {
            float t = (float)row / h;
            uint32_t r = (uint32_t)(((current_wallpaper.color1 >> 16) & 0xFF) * (1 - t) +
                                     ((current_wallpaper.color2 >> 16) & 0xFF) * t);
            uint32_t g = (uint32_t)(((current_wallpaper.color1 >> 8) & 0xFF) * (1 - t) +
                                     ((current_wallpaper.color2 >> 8) & 0xFF) * t);
            uint32_t b = (uint32_t)((current_wallpaper.color1 & 0xFF) * (1 - t) +
                                     (current_wallpaper.color2 & 0xFF) * t);
            uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;

            for (int col = 0; col < w; col++) {
                buffer[row * w + col] = color;
            }
        }
    } else if (current_wallpaper.type == WP_TYPE_SOLID) {
        for (int i = 0; i < w * h; i++) {
            buffer[i] = current_wallpaper.color1;
        }
    }

    // Render particles
    for (int i = 0; i < current_wallpaper.particle_count; i++) {
        wp_particle_t *p = &current_wallpaper.particles[i];
        int px = (int)p->x;
        int py = (int)p->y;
        int ps = (int)p->size;

        if (px >= 0 && px < w && py >= 0 && py < h) {
            uint32_t alpha = (uint32_t)(255 * p->life / p->max_life);
            uint32_t pcolor = (alpha << 24) | (p->color & 0xFFFFFF);

            for (int dy = -ps; dy <= ps; dy++) {
                for (int dx = -ps; dx <= ps; dx++) {
                    int cx = px + dx, cy = py + dy;
                    if (cx >= 0 && cx < w && cy >= 0 && cy < h) {
                        if (dx * dx + dy * dy <= ps * ps) {
                            buffer[cy * w + cx] = pcolor;
                        }
                    }
                }
            }
        }
    }
}

void wallpaper_update(float dt) {
    if (!current_wallpaper.animated) return;

    for (int i = 0; i < current_wallpaper.particle_count; i++) {
        wp_particle_t *p = &current_wallpaper.particles[i];
        p->x += p->vx * current_wallpaper.animation_speed;
        p->y += p->vy * current_wallpaper.animation_speed;
        p->life -= dt * 0.1f;

        if (p->life <= 0) {
            p->x = (float)(i * 37) % wp_width;
            p->y = (float)(i * 23) % wp_height;
            p->life = p->max_life;
            p->vx = ((float)(i % 5) - 2.0f) * 0.5f;
            p->vy = ((float)(i % 3) - 1.0f) * 0.3f;
        }

        // Wrap around
        if (p->x < 0) p->x += wp_width;
        if (p->x >= wp_width) p->x -= wp_width;
        if (p->y < 0) p->y += wp_height;
        if (p->y >= wp_height) p->y -= wp_height;
    }
}

void wallpaper_set_type(wp_type_t type) { current_wallpaper.type = type; }
void wallpaper_set_colors(uint32_t c1, uint32_t c2, uint32_t c3) {
    current_wallpaper.color1 = c1;
    current_wallpaper.color2 = c2;
    current_wallpaper.color3 = c3;
}
void wallpaper_set_image(const char *path) { strcpy(current_wallpaper.image_path, path); }
void wallpaper_set_animated(int animated) { current_wallpaper.animated = animated; }
wallpaper_t* wallpaper_get_current(void) { return &current_wallpaper; }