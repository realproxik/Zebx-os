// ============================================================================
// ZEBX OS - POST-PROCESSING EFFECTS
// Part 4: Bloom, SSAO, Motion Blur, Tone Mapping
// ============================================================================

#include "graphics3d.h"

// ============================================================================
// BLOOM EFFECT (Glow around bright areas)
// ============================================================================

void gfx3d_apply_bloom(framebuffer_t *fb, float intensity) {
    if (!fb) return;
    
    uint32_t w = fb->width;
    uint32_t h = fb->height;
    
    // Extract bright pixels
    static uint32_t *bright = NULL;
    static uint32_t *blur_h = NULL;
    static uint32_t *blur_v = NULL;
    
    if (!bright) {
        bright = (uint32_t*)kmalloc(w * h * 4);
        blur_h = (uint32_t*)kmalloc(w * h * 4);
        blur_v = (uint32_t*)kmalloc(w * h * 4);
    }
    
    // Threshold: extract pixels with brightness > 0.8
    float threshold = 0.8f;
    
    for (uint32_t i = 0; i < w * h; i++) {
        uint32_t c = fb->color[i];
        float r = ((c >> 16) & 0xFF) / 255.0f;
        float g = ((c >> 8) & 0xFF) / 255.0f;
        float b = (c & 0xFF) / 255.0f;
        float brightness = r * 0.299f + g * 0.587f + b * 0.114f;
        
        if (brightness > threshold) {
            float scale = (brightness - threshold) / (1.0f - threshold);
            uint32_t nr = (uint32_t)(r * scale * 255);
            uint32_t ng = (uint32_t)(g * scale * 255);
            uint32_t nb = (uint32_t)(b * scale * 255);
            bright[i] = 0xFF000000 | (nr << 16) | (ng << 8) | nb;
        } else {
            bright[i] = 0;
        }
    }
    
    // Horizontal blur (box blur, 9-tap)
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 4; x < w - 4; x++) {
            uint32_t r = 0, g = 0, b = 0, a = 0;
            for (int k = -4; k <= 4; k++) {
                uint32_t c = bright[y * w + x + k];
                r += (c >> 16) & 0xFF;
                g += (c >> 8) & 0xFF;
                b += c & 0xFF;
                a += (c >> 24) & 0xFF;
            }
            r /= 9; g /= 9; b /= 9; a /= 9;
            blur_h[y * w + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    // Vertical blur
    for (uint32_t y = 4; y < h - 4; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t r = 0, g = 0, b = 0, a = 0;
            for (int k = -4; k <= 4; k++) {
                uint32_t c = blur_h[(y + k) * w + x];
                r += (c >> 16) & 0xFF;
                g += (c >> 8) & 0xFF;
                b += c & 0xFF;
                a += (c >> 24) & 0xFF;
            }
            r /= 9; g /= 9; b /= 9; a /= 9;
            blur_v[y * w + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    // Additive blend: original + bloom * intensity
    float bloom_scale = intensity;
    
    for (uint32_t i = 0; i < w * h; i++) {
        uint32_t orig = fb->color[i];
        uint32_t bloom = blur_v[i];
        
        uint32_t r = (orig >> 16) & 0xFF;
        uint32_t g = (orig >> 8) & 0xFF;
        uint32_t b = orig & 0xFF;
        
        uint32_t br = (bloom >> 16) & 0xFF;
        uint32_t bg = (bloom >> 8) & 0xFF;
        uint32_t bb = bloom & 0xFF;
        
        r = (uint32_t)(r + br * bloom_scale);
        g = (uint32_t)(g + bg * bloom_scale);
        b = (uint32_t)(b + bb * bloom_scale);
        
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        
        fb->color[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

// ============================================================================
// TONE MAPPING (HDR to LDR)
// ============================================================================

void gfx3d_apply_tonemap(framebuffer_t *fb, float exposure) {
    if (!fb) return;
    
    uint32_t pixels = fb->width * fb->height;
    
    for (uint32_t i = 0; i < pixels; i++) {
        uint32_t c = fb->color[i];
        
        float r = ((c >> 16) & 0xFF) / 255.0f;
        float g = ((c >> 8) & 0xFF) / 255.0f;
        float b = (c & 0xFF) / 255.0f;
        
        // Reinhard tone mapping
        r = r * exposure;
        g = g * exposure;
        b = b * exposure;
        
        r = r / (1.0f + r);
        g = g / (1.0f + g);
        b = b / (1.0f + b);
        
        // Gamma correction (2.2)
        r = powf(r, 1.0f / 2.2f);
        g = powf(g, 1.0f / 2.2f);
        b = powf(b, 1.0f / 2.2f);
        
        uint32_t nr = (uint32_t)(r * 255);
        uint32_t ng = (uint32_t)(g * 255);
        uint32_t nb = (uint32_t)(b * 255);
        
        fb->color[i] = 0xFF000000 | (nr << 16) | (ng << 8) | nb;
    }
}

// ============================================================================
// MOTION BLUR (simple)
// ============================================================================

static uint32_t *prev_frame = NULL;

void gfx3d_apply_motion_blur(framebuffer_t *fb, float strength) {
    if (!fb) return;
    
    uint32_t pixels = fb->width * fb->height;
    
    if (!prev_frame) {
        prev_frame = (uint32_t*)kmalloc(pixels * 4);
        memcpy(prev_frame, fb->color, pixels * 4);
        return;
    }
    
    float curr_weight = 1.0f - strength;
    float prev_weight = strength;
    
    for (uint32_t i = 0; i < pixels; i++) {
        uint32_t c = fb->color[i];
        uint32_t p = prev_frame[i];
        
        uint32_t r = (uint32_t)(((c >> 16) & 0xFF) * curr_weight + ((p >> 16) & 0xFF) * prev_weight);
        uint32_t g = (uint32_t)(((c >> 8) & 0xFF) * curr_weight + ((p >> 8) & 0xFF) * prev_weight);
        uint32_t b = (uint32_t)((c & 0xFF) * curr_weight + (p & 0xFF) * prev_weight);
        
        fb->color[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
    
    memcpy(prev_frame, fb->color, pixels * 4);
}

// ============================================================================
// SSAO (Screen Space Ambient Occlusion) - Simplified
// ============================================================================

void gfx3d_apply_ssao(framebuffer_t *fb, float radius, float intensity) {
    if (!fb || !fb->depth) return;
    
    uint32_t w = fb->width;
    uint32_t h = fb->height;
    
    static uint32_t *ao_buffer = NULL;
    if (!ao_buffer) {
        ao_buffer = (uint32_t*)kmalloc(w * h * 4);
    }
    
    // Sample kernel (8 samples)
    vec2_t samples[8] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {0.707f, 0.707f}, {-0.707f, 0.707f}, {0.707f, -0.707f}, {-0.707f, -0.707f}
    };
    
    for (uint32_t y = 2; y < h - 2; y++) {
        for (uint32_t x = 2; x < w - 2; x++) {
            uint32_t idx = y * w + x;
            float center_depth = fb->depth[idx];
            
            if (center_depth >= 1.0f) {
                ao_buffer[idx] = 0xFF808080; // No AO for sky
                continue;
            }
            
            float occlusion = 0.0f;
            
            for (int i = 0; i < 8; i++) {
                int sx = (int)(x + samples[i].x * radius);
                int sy = (int)(y + samples[i].y * radius);
                
                if (sx >= 0 && sx < (int)w && sy >= 0 && sy < (int)h) {
                    float sample_depth = fb->depth[sy * w + sx];
                    
                    // If sample is closer than center, it occludes
                    float diff = sample_depth - center_depth;
                    if (diff < 0 && diff > -0.1f) {
                        occlusion += 1.0f;
                    }
                }
            }
            
            occlusion = 1.0f - (occlusion / 8.0f) * intensity;
            if (occlusion < 0) occlusion = 0;
            if (occlusion > 1) occlusion = 1;
            
            uint32_t ao = (uint32_t)(occlusion * 128 + 128);
            ao_buffer[idx] = 0xFF000000 | (ao << 16) | (ao << 8) | ao;
        }
    }
    
    // Apply AO to color
    for (uint32_t i = 0; i < w * h; i++) {
        uint32_t c = fb->color[i];
        uint32_t ao = ao_buffer[i] & 0xFF;
        
        float ao_factor = ao / 255.0f;
        
        uint32_t r = (uint32_t)(((c >> 16) & 0xFF) * ao_factor);
        uint32_t g = (uint32_t)(((c >> 8) & 0xFF) * ao_factor);
        uint32_t b = (uint32_t)((c & 0xFF) * ao_factor);
        
        fb->color[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}