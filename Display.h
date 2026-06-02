// ============================================================================
 // ZEBX OS - Display implement by jatin labs
 // ============================================================================
#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

#define MAX_DISPLAYS 8
#define MAX_RESOLUTIONS 32
#define DISPLAY_NAME_LEN 32

// Display types
typedef enum {
    DISPLAY_INTERNAL,   // Laptop panel
    DISPLAY_EXTERNAL,   // Desktop monitor
    DISPLAY_TV,         // Television
    DISPLAY_PROJECTOR,  // Projector
    DISPLAY_VR,         // VR headset
} display_type_t;

// Connector types
typedef enum {
    CONNECTOR_VGA,
    CONNECTOR_DVI,
    CONNECTOR_HDMI,
    CONNECTOR_DP,
    CONNECTOR_EDP,      // Embedded DisplayPort
    CONNECTOR_USBC_DP,
    CONNECTOR_WIRELESS,
} connector_type_t;

// Color formats
typedef enum {
    COLOR_RGB,
    COLOR_YUV444,
    COLOR_YUV422,
    COLOR_YUV420,
} color_format_t;

// HDR metadata
typedef struct {
    int supported;
    int enabled;
    uint16_t max_luminance;      // nits
    uint16_t min_luminance;      // nits * 10000
    uint16_t max_content_light;  // nits
    uint16_t max_frame_avg_light; // nits
    uint8_t color_gamut;         // 0=BT.709, 1=DCI-P3, 2=BT.2020
} hdr_info_t;

// Resolution info
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t refresh_hz;
    uint32_t pixel_clock_khz;
    int interlaced;
    int preferred;
} resolution_t;

// EDID data
typedef struct {
    uint8_t raw[256];
    char manufacturer[4];
    char model_name[16];
    char serial[16];
    uint16_t product_code;
    uint32_t serial_number;
    uint8_t week;
    uint8_t year;
    uint8_t version;
    uint8_t revision;
    uint32_t horizontal_cm;
    uint32_t vertical_cm;
    uint32_t gamma;
    uint32_t dpms_standby : 1;
    uint32_t dpms_suspend : 1;
    uint32_t dpms_active_off : 1;
    uint32_t digital : 1;
    uint32_t color_depth;
    resolution_t resolutions[MAX_RESOLUTIONS];
    uint32_t num_resolutions;
} edid_info_t;

// Display info
typedef struct {
    uint32_t display_id;
    char name[DISPLAY_NAME_LEN];
    display_type_t type;
    connector_type_t connector;
    
    // Current mode
    uint32_t width;
    uint32_t height;
    uint32_t refresh_hz;
    uint32_t bpp;
    color_format_t color_format;
    
    // Position in desktop
    int32_t x;
    int32_t y;
    int32_t primary;
    int32_t enabled;
    
    // Hardware
    uint32_t framebuffer_phys;
    uint32_t framebuffer_size;
    uint32_t *framebuffer_virt;
    uint32_t pitch;
    
    // EDID
    edid_info_t edid;
    
    // HDR
    hdr_info_t hdr;
    
    // VRR (Variable Refresh Rate)
    int vrr_supported;
    int vrr_enabled;
    uint32_t vrr_min_hz;
    uint32_t vrr_max_hz;
    
    // DPI
    uint32_t dpi_x;
    uint32_t dpi_y;
    
    // Color calibration
    uint8_t gamma_r[256];
    uint8_t gamma_g[256];
    uint8_t gamma_b[256];
    float brightness;
    float contrast;
    float saturation;
    
    // Power
    int dpms_state; // 0=on, 1=standby, 2=suspend, 3=off
    
} display_info_t;

// Display manager functions
void display_init(void);
int display_detect(void);
int display_add(uint32_t display_id, connector_type_t connector);
int display_remove(uint32_t display_id);
int display_set_mode(uint32_t display_id, uint32_t width, uint32_t height, uint32_t refresh);
int display_set_position(uint32_t display_id, int32_t x, int32_t y);
int display_set_primary(uint32_t display_id);
int display_enable(uint32_t display_id);
int display_disable(uint32_t display_id);

// EDID
int display_read_edid(uint32_t display_id);
int display_parse_edid(uint8_t *raw, edid_info_t *edid);

// HDR
int display_enable_hdr(uint32_t display_id);
int display_disable_hdr(uint32_t display_id);
void display_set_hdr_metadata(uint32_t display_id, hdr_info_t *hdr);

// VRR
int display_enable_vrr(uint32_t display_id);
int display_disable_vrr(uint32_t display_id);

// Color
void display_set_gamma(uint32_t display_id, uint8_t *r, uint8_t *g, uint8_t *b);
void display_set_brightness(uint32_t display_id, float brightness);
void display_set_contrast(uint32_t display_id, float contrast);

// DPMS
void display_dpms_on(uint32_t display_id);
void display_dpms_standby(uint32_t display_id);
void display_dpms_suspend(uint32_t display_id);
void display_dpms_off(uint32_t display_id);

// Info
display_info_t* display_get_info(uint32_t display_id);
uint32_t display_get_count(void);
display_info_t* display_get_primary(void);

// Drawing
void display_clear(uint32_t display_id, uint32_t color);
void display_put_pixel(uint32_t display_id, int x, int y, uint32_t color);
void display_fill_rect(uint32_t display_id, int x, int y, int w, int h, uint32_t color);
void display_blit(uint32_t display_id, uint32_t *src, int x, int y, int w, int h);
void display_flip(uint32_t display_id);

// Multi-display desktop
void display_arrange_horizontal(void);
void display_arrange_vertical(void);
void display_arrange_mirror(uint32_t master_id);
void display_arrange_extend(void);

#endif

#include "display.h"
#include "vga.h"
#include "memory.h"
#include "simd.h"

static display_info_t displays[MAX_DISPLAYS];
static uint32_t display_count = 0;
static uint32_t primary_display = 0;

// VBE / GOP constants
#define VBE_FRAMEBUFFER 0xE0000000
#define VBE_MODE_800x600x32 0x4118
#define VBE_MODE_1024x768x32 0x4119
#define VBE_MODE_1280x720x32 0x411A
#define VBE_MODE_1920x1080x32 0x411B
#define VBE_MODE_2560x1440x32 0x411C
#define VBE_MODE_3840x2160x32 0x411D

// I2C/DDC for EDID
#define DDC_SCL 0x3C0
#define DDC_SDA 0x3C1

static inline void ddc_delay(void) {
    for (volatile int i = 0; i < 100; i++);
}

static void ddc_start(void) {
    // Simplified I2C start condition
}

static void ddc_stop(void) {
    // Simplified I2C stop condition
}

static uint8_t ddc_read_byte(int ack) {
    return 0;
}

static int ddc_write_byte(uint8_t data) {
    return 1;
}

int display_read_edid(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    
    display_info_t *disp = &displays[display_id];
    
    // Read EDID via DDC2
    ddc_start();
    if (!ddc_write_byte(0xA0)) { // EDID address
        ddc_stop();
        return -1;
    }
    if (!ddc_write_byte(0x00)) { // Start at offset 0
        ddc_stop();
        return -1;
    }
    
    ddc_start();
    if (!ddc_write_byte(0xA1)) { // Read
        ddc_stop();
        return -1;
    }
    
    for (int i = 0; i < 128; i++) {
        disp->edid.raw[i] = ddc_read_byte(i < 127);
    }
    ddc_stop();
    
    // Extended EDID (256 bytes)
    ddc_start();
    ddc_write_byte(0xA0);
    ddc_write_byte(0x80);
    ddc_start();
    ddc_write_byte(0xA1);
    for (int i = 128; i < 256; i++) {
        disp->edid.raw[i] = ddc_read_byte(i < 255);
    }
    ddc_stop();
    
    return display_parse_edid(disp->edid.raw, &disp->edid);
}

int display_parse_edid(uint8_t *raw, edid_info_t *edid) {
    // Verify header
    if (raw[0] != 0x00 || raw[1] != 0xFF || raw[2] != 0xFF || raw[3] != 0xFF ||
        raw[4] != 0xFF || raw[5] != 0xFF || raw[6] != 0xFF || raw[7] != 0x00) {
        return -1;
    }
    
    // Manufacturer
    uint16_t manu = (raw[8] << 8) | raw[9];
    edid->manufacturer[0] = 'A' + ((manu >> 10) & 0x1F) - 1;
    edid->manufacturer[1] = 'A' + ((manu >> 5) & 0x1F) - 1;
    edid->manufacturer[2] = 'A' + (manu & 0x1F) - 1;
    edid->manufacturer[3] = '\\0';
    
    edid->product_code = raw[10] | (raw[11] << 8);
    edid->serial_number = raw[12] | (raw[13] << 8) | (raw[14] << 16) | (raw[15] << 24);
    edid->week = raw[16];
    edid->year = raw[17] + 1990;
    edid->version = raw[18];
    edid->revision = raw[19];
    
    // Display parameters
    edid->horizontal_cm = raw[21];
    edid->vertical_cm = raw[22];
    edid->gamma = raw[23] == 0xFF ? 0 : (raw[23] + 100) / 100;
    edid->dpms_standby = (raw[24] >> 7) & 1;
    edid->dpms_suspend = (raw[24] >> 6) & 1;
    edid->dpms_active_off = (raw[24] >> 5) & 1;
    edid->digital = (raw[20] >> 7) & 1;
    
    // Parse detailed timing descriptors
    edid->num_resolutions = 0;
    
    for (int i = 0; i < 4; i++) {
        int offset = 54 + i * 18;
        
        uint16_t pixel_clock = raw[offset] | (raw[offset + 1] << 8);
        if (pixel_clock == 0) continue; // Not a timing descriptor
        
        uint16_t h_active = raw[offset + 2] | ((raw[offset + 4] >> 4) << 8);
        uint16_t h_blank = raw[offset + 3] | ((raw[offset + 4] & 0x0F) << 8);
        uint16_t v_active = raw[offset + 5] | ((raw[offset + 7] >> 4) << 8);
        uint16_t v_blank = raw[offset + 6] | ((raw[offset + 7] & 0x0F) << 8);
        
        resolution_t *res = &edid->resolutions[edid->num_resolutions++];
        res->width = h_active;
        res->height = v_active;
        res->pixel_clock_khz = pixel_clock * 10;
        res->interlaced = (raw[offset + 17] >> 7) & 1;
        res->preferred = (i == 0);
        
        // Calculate refresh rate
        uint32_t h_total = h_active + h_blank;
        uint32_t v_total = v_active + v_blank;
        res->refresh_hz = (res->pixel_clock_khz * 1000) / (h_total * v_total);
        
        if (edid->num_resolutions >= MAX_RESOLUTIONS) break;
    }
    
    // Standard timings
    for (int i = 0; i < 8; i++) {
        int offset = 38 + i * 2;
        if (raw[offset] == 0x01 && raw[offset + 1] == 0x01) continue;
        
        uint16_t h = (raw[offset] + 31) * 8;
        uint16_t ratio = (raw[offset + 1] >> 6) & 0x3;
        uint16_t v;
        
        switch (ratio) {
            case 0: v = h * 10 / 16; break;
            case 1: v = h * 3 / 4; break;
            case 2: v = h * 4 / 5; break;
            case 3: v = h * 9 / 16; break;
        }
        
        uint16_t refresh = (raw[offset + 1] & 0x3F) + 60;
        
        if (edid->num_resolutions < MAX_RESOLUTIONS) {
            resolution_t *res = &edid->resolutions[edid->num_resolutions++];
            res->width = h;
            res->height = v;
            res->refresh_hz = refresh;
            res->pixel_clock_khz = 0;
            res->interlaced = 0;
            res->preferred = 0;
        }
    }
    
    // Model name from descriptor
    for (int i = 0; i < 4; i++) {
        int offset = 54 + i * 18;
        if (raw[offset + 3] == 0xFC) { // Monitor name
            for (int j = 0; j < 13; j++) {
                char c = raw[offset + 5 + j];
                if (c == 0x0A) {
                    edid->model_name[j] = '\\0';
                    break;
                }
                edid->model_name[j] = c;
            }
            edid->model_name[13] = '\\0';
            break;
        }
    }
    
    // Serial number from descriptor
    for (int i = 0; i < 4; i++) {
        int offset = 54 + i * 18;
        if (raw[offset + 3] == 0xFF) { // Serial number
            for (int j = 0; j < 13; j++) {
                char c = raw[offset + 5 + j];
                if (c == 0x0A) {
                    edid->serial[j] = '\\0';
                    break;
                }
                edid->serial[j] = c;
            }
            edid->serial[13] = '\\0';
            break;
        }
    }
    
    return 0;
}

void display_init(void) {
    memset(displays, 0, sizeof(displays));
    display_count = 0;
    primary_display = 0;
    
    vga_puts("[+] Display Manager initialized\\n");
    
    // Detect displays
    display_detect();
}

int display_detect(void) {
    // Primary display (VBE framebuffer)
    if (display_count < MAX_DISPLAYS) {
        display_info_t *disp = &displays[display_count];
        disp->display_id = display_count;
        strcpy(disp->name, "Primary Display");
        disp->type = DISPLAY_EXTERNAL;
        disp->connector = CONNECTOR_DP;
        disp->width = 1920;
        disp->height = 1080;
        disp->refresh_hz = 60;
        disp->bpp = 32;
        disp->color_format = COLOR_RGB;
        disp->x = 0;
        disp->y = 0;
        disp->primary = 1;
        disp->enabled = 1;
        disp->framebuffer_phys = VBE_FRAMEBUFFER;
        disp->framebuffer_size = 1920 * 1080 * 4;
        disp->framebuffer_virt = (uint32_t*)VBE_FRAMEBUFFER;
        disp->pitch = 1920 * 4;
        disp->dpi_x = 96;
        disp->dpi_y = 96;
        disp->brightness = 1.0f;
        disp->contrast = 1.0f;
        disp->saturation = 1.0f;
        disp->dpms_state = 0;
        
        // Default gamma
        for (int i = 0; i < 256; i++) {
            disp->gamma_r[i] = i;
            disp->gamma_g[i] = i;
            disp->gamma_b[i] = i;
        }
        
        // Try to read EDID
        display_read_edid(display_count);
        
        // Detect HDR from EDID
        if (disp->edid.raw[128] == 0x02 && disp->edid.raw[129] == 0x03) {
            // CEA-861 extension block
            uint8_t *cea = &disp->edid.raw[128];
            uint8_t dtd_offset = cea[2];
            
            for (int i = 4; i < dtd_offset; i += (cea[i] & 0x1F) + 1) {
                uint8_t tag = (cea[i] >> 5) & 0x7;
                if (tag == 7) { // Extended tag
                    uint8_t ext_tag = cea[i + 1];
                    if (ext_tag == 6 || ext_tag == 7) { // HDR static/dynamic
                        disp->hdr.supported = 1;
                        disp->hdr.max_luminance = cea[i + 2] * 50 + 50;
                        disp->hdr.min_luminance = cea[i + 3] * 0.0001;
                        disp->hdr.max_content_light = (cea[i + 4] << 8) | cea[i + 5];
                        disp->hdr.max_frame_avg_light = (cea[i + 6] << 8) | cea[i + 7];
                    }
                }
            }
        }
        
        // VRR detection (from DisplayID or CEA)
        disp->vrr_supported = 1; // Assume supported for DP
        disp->vrr_min_hz = 30;
        disp->vrr_max_hz = 144;
        
        display_count++;
        
        vga_puts("    Display 0: ");
        vga_put_int(disp->width);
        vga_puts("x");
        vga_put_int(disp->height);
        vga_puts("@");
        vga_put_int(disp->refresh_hz);
        vga_puts("Hz");
        if (disp->hdr.supported) vga_puts(" HDR");
        if (disp->vrr_supported) vga_puts(" VRR");
        vga_puts("\\n");
        
        if (strlen(disp->edid.model_name) > 0) {
            vga_puts("    Model: ");
            vga_puts(disp->edid.model_name);
            vga_puts("\\n");
        }
    }
    
    return display_count;
}

int display_set_mode(uint32_t display_id, uint32_t width, uint32_t height, uint32_t refresh) {
    if (display_id >= display_count) return -1;
    
    display_info_t *disp = &displays[display_id];
    
    // Validate mode against EDID
    int found = 0;
    for (uint32_t i = 0; i < disp->edid.num_resolutions; i++) {
        if (disp->edid.resolutions[i].width == width &&
            disp->edid.resolutions[i].height == height &&
            disp->edid.resolutions[i].refresh_hz == refresh) {
            found = 1;
            break;
        }
    }
    
    if (!found && disp->edid.num_resolutions > 0) {
        // Use closest available
        width = disp->edid.resolutions[0].width;
        height = disp->edid.resolutions[0].height;
        refresh = disp->edid.resolutions[0].refresh_hz;
    }
    
    disp->width = width;
    disp->height = height;
    disp->refresh_hz = refresh;
    disp->pitch = width * (disp->bpp / 8);
    disp->framebuffer_size = width * height * (disp->bpp / 8);
    
    // Reallocate framebuffer if needed
    if (disp->framebuffer_virt) {
        // In real implementation, would remap framebuffer
    }
    
    return 0;
}

int display_set_position(uint32_t display_id, int32_t x, int32_t y) {
    if (display_id >= display_count) return -1;
    displays[display_id].x = x;
    displays[display_id].y = y;
    return 0;
}

int display_set_primary(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    
    for (uint32_t i = 0; i < display_count; i++) {
        displays[i].primary = 0;
    }
    
    displays[display_id].primary = 1;
    primary_display = display_id;
    return 0;
}

int display_enable(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    displays[display_id].enabled = 1;
    return 0;
}

int display_disable(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    displays[display_id].enabled = 0;
    return 0;
}

int display_enable_hdr(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    if (!displays[display_id].hdr.supported) return -1;
    
    displays[display_id].hdr.enabled = 1;
    
    // Send HDR metadata infoframe
    // In real implementation, would program DP/HDMI infoframe
    
    return 0;
}

int display_disable_hdr(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    displays[display_id].hdr.enabled = 0;
    return 0;
}

void display_set_hdr_metadata(uint32_t display_id, hdr_info_t *hdr) {
    if (display_id >= display_count) return;
    memcpy(&displays[display_id].hdr, hdr, sizeof(hdr_info_t));
}

int display_enable_vrr(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    if (!displays[display_id].vrr_supported) return -1;
    
    displays[display_id].vrr_enabled = 1;
    
    // Enable Adaptive-Sync / FreeSync / G-Sync
    // Program DP MSA and DPCD
    
    return 0;
}

int display_disable_vrr(uint32_t display_id) {
    if (display_id >= display_count) return -1;
    displays[display_id].vrr_enabled = 0;
    return 0;
}

void display_set_gamma(uint32_t display_id, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (display_id >= display_count) return;
    
    display_info_t *disp = &displays[display_id];
    memcpy(disp->gamma_r, r, 256);
    memcpy(disp->gamma_g, g, 256);
    memcpy(disp->gamma_b, b, 256);
    
    // Apply gamma LUT to hardware
    // In real implementation, would program display controller gamma LUT
}

void display_set_brightness(uint32_t display_id, float brightness) {
    if (display_id >= display_count) return;
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 2.0f) brightness = 2.0f;
    
    displays[display_id].brightness = brightness;
    
    // Apply brightness via DP AUX or DDC/CI
    // Or modify gamma LUT
}

void display_set_contrast(uint32_t display_id, float contrast) {
    if (display_id >= display_count) return;
    if (contrast < 0.0f) contrast = 0.0f;
    if (contrast > 2.0f) contrast = 2.0f;
    
    displays[display_id].contrast = contrast;
}

void display_dpms_on(uint32_t display_id) {
    if (display_id >= display_count) return;
    displays[display_id].dpms_state = 0;
    displays[display_id].enabled = 1;
}

void display_dpms_standby(uint32_t display_id) {
    if (display_id >= display_count) return;
    displays[display_id].dpms_state = 1;
}

void display_dpms_suspend(uint32_t display_id) {
    if (display_id >= display_count) return;
    displays[display_id].dpms_state = 2;
}

void display_dpms_off(uint32_t display_id) {
    if (display_id >= display_count) return;
    displays[display_id].dpms_state = 3;
    displays[display_id].enabled = 0;
}

display_info_t* display_get_info(uint32_t display_id) {
    if (display_id >= display_count) return NULL;
    return &displays[display_id];
}

uint32_t display_get_count(void) { return display_count; }

display_info_t* display_get_primary(void) {
    if (display_count == 0) return NULL;
    return &displays[primary_display];
}

void display_clear(uint32_t display_id, uint32_t color) {
    if (display_id >= display_count) return;
    
    display_info_t *disp = &displays[display_id];
    if (!disp->enabled || !disp->framebuffer_virt) return;
    
    uint32_t *fb = disp->framebuffer_virt;
    uint32_t pixels = disp->width * disp->height;
    
    // Use SIMD for fast clear
    if (simd_has_feature(SIMD_FEATURE_AVX512F) && pixels >= 16) {
        v16si color_vec = {color, color, color, color, color, color, color, color,
                           color, color, color, color, color, color, color, color};
        uint32_t i = 0;
        for (; i + 16 <= pixels; i += 16) {
            __asm__ volatile(
                "vmovdqu64 %0, (%1)"
                :
                : "x"(color_vec), "r"(&fb[i])
                : "memory"
            );
        }
        for (; i < pixels; i++) fb[i] = color;
    } else {
        for (uint32_t i = 0; i < pixels; i++) {
            fb[i] = color;
        }
    }
}

void display_put_pixel(uint32_t display_id, int x, int y, uint32_t color) {
    if (display_id >= display_count) return;
    
    display_info_t *disp = &displays[display_id];
    if (!disp->enabled || x < 0 || y < 0 || x >= (int)disp->width || y >= (int)disp->height) return;
    
    disp->framebuffer_virt[y * (disp->pitch / 4) + x] = color;
}

void display_fill_rect(uint32_t display_id, int x, int y, int w, int h, uint32_t color) {
    if (display_id >= display_count) return;
    
    display_info_t *disp = &displays[display_id];
    if (!disp->enabled) return;
    
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > (int)disp->width ? disp->width : x + w;
    int y2 = y + h > (int)disp->height ? disp->height : y + h;
    
    uint32_t *fb = disp->framebuffer_virt;
    uint32_t pitch_pixels = disp->pitch / 4;
    
    for (int row = y1; row < y2; row++) {
        for (int col = x1; col < x2; col++) {
            fb[row * pitch_pixels + col] = color;
        }
    }
}

void display_blit(uint32_t display_id, uint32_t *src, int x, int y, int w, int h) {
    if (display_id >= display_count) return;
    
    display_info_t *disp = &displays[display_id];
    if (!disp->enabled) return;
    
    uint32_t *fb = disp->framebuffer_virt;
    uint32_t pitch_pixels = disp->pitch / 4;
    
    for (int row = 0; row < h; row++) {
        int dst_y = y + row;
        if (dst_y < 0 || dst_y >= (int)disp->height) continue;
        
        for (int col = 0; col < w; col++) {
            int dst_x = x + col;
            if (dst_x < 0 || dst_x >= (int)disp->width) continue;
            
            fb[dst_y * pitch_pixels + dst_x] = src[row * w + col];
        }
    }
}

void display_flip(uint32_t display_id) {
    if (display_id >= display_count) return;
    // In real implementation, would flip front/back buffers
    // For now, framebuffer is direct
}

void display_arrange_horizontal(void) {
    int x = 0;
    for (uint32_t i = 0; i < display_count; i++) {
        displays[i].x = x;
        displays[i].y = 0;
        x += displays[i].width;
    }
}

void display_arrange_vertical(void) {
    int y = 0;
    for (uint32_t i = 0; i < display_count; i++) {
        displays[i].x = 0;
        displays[i].y = y;
        y += displays[i].height;
    }
}

void display_arrange_mirror(uint32_t master_id) {
    if (master_id >= display_count) return;
    
    for (uint32_t i = 0; i < display_count; i++) {
        if (i != master_id) {
            displays[i].x = displays[master_id].x;
            displays[i].y = displays[master_id].y;
            displays[i].width = displays[master_id].width;
            displays[i].height = displays[master_id].height;
        }
    }
}

void display_arrange_extend(void) {
    display_arrange_horizontal();
}