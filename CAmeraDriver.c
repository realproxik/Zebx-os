#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"

#define CAMERA_MAX_RESOLUTION 8192  // 50MP = ~8192x6144
#define CAMERA_BUFFER_SIZE (50 * 1024 * 1024)  // 50MB buffer

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t fps;
    uint32_t zoom_level;
    uint32_t iso;
    uint32_t shutter_speed;
    uint32_t aperture;
    int flash_enabled;
    int hdr_enabled;
    int night_mode;
    int portrait_mode;
} camera_config_t;

void camera_init(void);
void camera_capture(void);
void camera_start_video(void);
void camera_stop_video(void);
void camera_set_resolution(uint32_t width, uint32_t height);
void camera_set_zoom(uint32_t level);
void camera_set_iso(uint32_t iso);
void camera_toggle_flash(void);
void camera_toggle_hdr(void);
void camera_save_photo(const char *filename);
uint8_t* camera_get_buffer(void);

#endif

// Supports up to 8192x6144 resolution (50MP)

#include "types.h"
#include "vga.h"
#include "memory.h"
#include "fs.h"

#define CAMERA_PORT_BASE 0x3000
#define CAMERA_CMD_REG 0x00
#define CAMERA_STATUS_REG 0x04
#define CAMERA_RES_REG 0x08
#define CAMERA_BUF_ADDR 0x0C
#define CAMERA_BUF_SIZE 0x10
#define CAMERA_ZOOM_REG 0x14
#define CAMERA_ISO_REG 0x18
#define CAMERA_FLASH_REG 0x1C
#define CAMERA_HDR_REG 0x20

static camera_config_t camera_config = {
    .width = 8192,
    .height = 6144,
    .bpp = 24,
    .fps = 60,
    .zoom_level = 1,
    .iso = 100,
    .shutter_speed = 1000,
    .aperture = 28,
    .flash_enabled = 0,
    .hdr_enabled = 1,
    .night_mode = 0,
    .portrait_mode = 0
};

static uint8_t *camera_buffer = NULL;
static int camera_recording = 0;
static uint32_t video_frames = 0;

void camera_init(void) {
    camera_buffer = (uint8_t *)kmalloc(CAMERA_BUFFER_SIZE);
    if (!camera_buffer) {
        vga_puts("[!] Camera: Failed to allocate buffer\\n");
        return;
    }
    
    // Initialize camera hardware
    outl(0x1, CAMERA_PORT_BASE + CAMERA_CMD_REG);  // Power on
    outl(0x50, CAMERA_PORT_BASE + CAMERA_RES_REG);  // 50MP mode
    outl((uint32_t)camera_buffer, CAMERA_PORT_BASE + CAMERA_BUF_ADDR);
    outl(CAMERA_BUFFER_SIZE, CAMERA_PORT_BASE + CAMERA_BUF_SIZE);
    
    vga_puts("[+] 50MP Camera initialized\\n");
    vga_puts("    Resolution: 8192x6144\\n");
    vga_puts("    FPS: 60\\n");
    vga_puts("    HDR: Enabled\\n");
}

void camera_capture(void) {
    // Trigger capture
    outl(0x2, CAMERA_PORT_BASE + CAMERA_CMD_REG);
    
    // Wait for capture complete
    while (inl(CAMERA_PORT_BASE + CAMERA_STATUS_REG) & 0x1);
    
    vga_puts("[+] Photo captured (50MP)\\n");
}

void camera_start_video(void) {
    camera_recording = 1;
    video_frames = 0;
    outl(0x3, CAMERA_PORT_BASE + CAMERA_CMD_REG);  // Start video
    vga_puts("[+] Video recording started (4K@60fps)\\n");
}

void camera_stop_video(void) {
    camera_recording = 0;
    outl(0x4, CAMERA_PORT_BASE + CAMERA_CMD_REG);  // Stop video
    vga_puts("[+] Video recording stopped\\n");
    vga_puts("    Total frames: ");
    vga_put_int(video_frames);
    vga_puts("\\n");
}

void camera_set_resolution(uint32_t width, uint32_t height) {
    camera_config.width = width;
    camera_config.height = height;
    outl((width << 16) | height, CAMERA_PORT_BASE + CAMERA_RES_REG);
}

void camera_set_zoom(uint32_t level) {
    camera_config.zoom_level = level;
    outl(level, CAMERA_PORT_BASE + CAMERA_ZOOM_REG);
}

void camera_set_iso(uint32_t iso) {
    camera_config.iso = iso;
    outl(iso, CAMERA_PORT_BASE + CAMERA_ISO_REG);
}

void camera_toggle_flash(void) {
    camera_config.flash_enabled = !camera_config.flash_enabled;
    outl(camera_config.flash_enabled, CAMERA_PORT_BASE + CAMERA_FLASH_REG);
}

void camera_toggle_hdr(void) {
    camera_config.hdr_enabled = !camera_config.hdr_enabled;
    outl(camera_config.hdr_enabled, CAMERA_PORT_BASE + CAMERA_HDR_REG);
}

void camera_save_photo(const char *filename) {
    // Save to 3GB storage
    int fd = fs_open(filename, O_CREAT | O_WRONLY);
    if (fd < 0) {
        vga_puts("[!] Failed to save photo\\n");
        return;
    }
    
    uint32_t size = camera_config.width * camera_config.height * (camera_config.bpp / 8);
    fs_write(fd, camera_buffer, size);
    fs_close(fd);
    
    vga_puts("[+] Photo saved: ");
    vga_puts(filename);
    vga_puts("\\n");
}

uint8_t* camera_get_buffer(void) {
    return camera_buffer;
}

static inline void outl(uint32_t val, uint16_t port) {
    asm volatile("outl %0, %1" : : "a"(val), "dN"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}