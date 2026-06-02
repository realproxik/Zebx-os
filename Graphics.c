// ============================================================================
// ZEBX OS - 3D GRAPHICS ENGINE IMPLEMENTATION
// Part 1: Math, Setup, Framebuffer
// ============================================================================

#include "graphics3d.h"
#include "vga.h"
#include "memory.h"
#include "simd.h"

static render_state_t render_state;
static vertex_t vertex_buffer[GFX_MAX_VERTICES];
static triangle_t triangle_buffer[GFX_MAX_TRIANGLES];
static uint32_t num_vertices = 0;
static uint32_t num_triangles = 0;
static texture_t *textures[GFX_MAX_TEXTURES];
static uint32_t num_textures = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void gfx3d_init(void) {
    memset(&render_state, 0, sizeof(render_state));
    
    // Default matrices
    mat4_identity(&render_state.model);
    mat4_identity(&render_state.view);
    mat4_identity(&render_state.projection);
    
    // Default viewport
    render_state.vp_x = 0;
    render_state.vp_y = 0;
    render_state.vp_w = GFX_BACKBUFFER_WIDTH;
    render_state.vp_h = GFX_BACKBUFFER_HEIGHT;
    
    // Default render flags
    render_state.flags = GFX_RENDER_ZBUFFER | GFX_RENDER_CULL_BACK | GFX_RENDER_GOURAUD;
    
    // Default clear values
    render_state.clear_color = 0xFF1A1A2E; // Dark blue
    render_state.clear_depth = 1.0f;
    render_state.clear_stencil = 0;
    
    // Default ambient
    render_state.ambient_color = (vec3_t){0.1f, 0.1f, 0.15f};
    render_state.ambient_intensity = 1.0f;
    
    // Create default framebuffer
    render_state.target = gfx3d_create_framebuffer(GFX_BACKBUFFER_WIDTH, GFX_BACKBUFFER_HEIGHT);
    
    vga_puts("[+] ZEBX 3D Graphics Engine initialized\n");
    vga_puts("    Resolution: ");
    vga_put_int(GFX_BACKBUFFER_WIDTH);
    vga_puts("x");
    vga_put_int(GFX_BACKBUFFER_HEIGHT);
    vga_puts("\n");
    vga_puts("    SIMD: ");
    if (simd_has_feature(SIMD_FEATURE_AVX512F)) {
        vga_puts("AVX-512 (512-bit)\n");
    } else if (simd_has_feature(SIMD_FEATURE_AVX2)) {
        vga_puts("AVX2 (256-bit)\n");
    } else if (simd_has_feature(SIMD_FEATURE_SSE42)) {
        vga_puts("SSE4.2 (128-bit)\n");
    } else {
        vga_puts("Scalar fallback\n");
    }
    vga_puts("    Max triangles: ");
    vga_put_int(GFX_MAX_TRIANGLES);
    vga_puts("\n");
    vga_puts("    Max vertices: ");
    vga_put_int(GFX_MAX_VERTICES);
    vga_puts("\n");
}

void gfx3d_shutdown(void) {
    if (render_state.target) {
        gfx3d_destroy_framebuffer(render_state.target);
    }
    for (uint32_t i = 0; i < num_textures; i++) {
        gfx3d_destroy_texture(textures[i]);
    }
}

render_state_t* gfx3d_get_state(void) {
    return &render_state;
}

// ============================================================================
// FRAMEBUFFER
// ============================================================================

framebuffer_t* gfx3d_create_framebuffer(uint32_t w, uint32_t h) {
    framebuffer_t *fb = (framebuffer_t*)kmalloc(sizeof(framebuffer_t));
    if (!fb) return NULL;
    
    fb->width = w;
    fb->height = h;
    fb->pitch = w;
    
    // Allocate aligned for SIMD
    uint32_t color_size = w * h * sizeof(uint32_t);
    uint32_t depth_size = w * h * sizeof(float);
    uint32_t stencil_size = w * h * sizeof(uint32_t);
    
    // Align to 64 bytes for AVX-512
    color_size = (color_size + 63) & ~63;
    depth_size = (depth_size + 63) & ~63;
    stencil_size = (stencil_size + 63) & ~63;
    
    fb->color = (uint32_t*)kmalloc(color_size);
    fb->depth = (float*)kmalloc(depth_size);
    fb->stencil = (uint32_t*)kmalloc(stencil_size);
    
    if (!fb->color || !fb->depth || !fb->stencil) {
        kfree(fb->color);
        kfree(fb->depth);
        kfree(fb->stencil);
        kfree(fb);
        return NULL;
    }
    
    memset(fb->color, 0, color_size);
    memset(fb->depth, 0, depth_size);
    memset(fb->stencil, 0, stencil_size);
    
    return fb;
}

void gfx3d_destroy_framebuffer(framebuffer_t *fb) {
    if (!fb) return;
    kfree(fb->color);
    kfree(fb->depth);
    kfree(fb->stencil);
    kfree(fb);
}

void gfx3d_set_framebuffer(framebuffer_t *fb) {
    render_state.target = fb;
}

void gfx3d_clear(uint32_t color, float depth, uint32_t stencil) {
    framebuffer_t *fb = render_state.target;
    if (!fb) return;
    
    uint32_t pixels = fb->width * fb->height;
    
    // SIMD-optimized clear
    if (simd_has_feature(SIMD_FEATURE_AVX512F) && pixels >= 16) {
        v16si color_vec = {color, color, color, color, color, color, color, color,
                          color, color, color, color, color, color, color, color};
        
        uint32_t i = 0;
        for (; i + 16 <= pixels; i += 16) {
            __asm__ volatile(
                "vmovdqu64 %0, (%1)"
                :
                : "x"(color_vec), "r"(&fb->color[i])
                : "memory"
            );
        }
        for (; i < pixels; i++) fb->color[i] = color;
        
        // Clear depth with AVX-512
        // Using integer store for float pattern (0x3F800000 = 1.0f)
        uint32_t depth_pattern = *((uint32_t*)&depth);
        v16si depth_vec = {depth_pattern, depth_pattern, depth_pattern, depth_pattern,
                           depth_pattern, depth_pattern, depth_pattern, depth_pattern,
                           depth_pattern, depth_pattern, depth_pattern, depth_pattern,
                           depth_pattern, depth_pattern, depth_pattern, depth_pattern};
        
        i = 0;
        for (; i + 16 <= pixels; i += 16) {
            __asm__ volatile(
                "vmovdqu64 %0, (%1)"
                :
                : "x"(depth_vec), "r"(&fb->depth[i])
                : "memory"
            );
        }
        for (; i < pixels; i++) fb->depth[i] = depth;
        
    } else if (simd_has_feature(SIMD_FEATURE_AVX2) && pixels >= 8) {
        v8si color_vec = {color, color, color, color, color, color, color, color};
        
        uint32_t i = 0;
        for (; i + 8 <= pixels; i += 8) {
            __asm__ volatile(
                "vmovdqu %0, (%1)"
                :
                : "x"(color_vec), "r"(&fb->color[i])
                : "memory"
            );
        }
        for (; i < pixels; i++) fb->color[i] = color;
        
        // Depth clear
        for (i = 0; i < pixels; i++) fb->depth[i] = depth;
        
    } else {
        // Scalar fallback
        for (uint32_t i = 0; i < pixels; i++) {
            fb->color[i] = color;
            fb->depth[i] = depth;
            fb->stencil[i] = stencil;
        }
    }
}

// ============================================================================
// VECTOR MATH
// ============================================================================

void vec3_set(vec3_t *v, float x, float y, float z) {
    v->x = x; v->y = y; v->z = z;
}

void vec3_add(vec3_t *out, vec3_t *a, vec3_t *b) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

void vec3_sub(vec3_t *out, vec3_t *a, vec3_t *b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

void vec3_mul(vec3_t *out, vec3_t *a, float s) {
    out->x = a->x * s;
    out->y = a->y * s;
    out->z = a->z * s;
}

float vec3_dot(vec3_t *a, vec3_t *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vec3_cross(vec3_t *out, vec3_t *a, vec3_t *b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

float vec3_len(vec3_t *v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

void vec3_normalize(vec3_t *v) {
    float len = vec3_len(v);
    if (len > 0.0001f) {
        float inv = 1.0f / len;
        v->x *= inv;
        v->y *= inv;
        v->z *= inv;
    }
}

// ============================================================================
// MATRIX MATH
// ============================================================================

void mat4_identity(mat4_t *m) {
    memset(m, 0, sizeof(mat4_t));
    m->m[0][0] = 1.0f;
    m->m[1][1] = 1.0f;
    m->m[2][2] = 1.0f;
    m->m[3][3] = 1.0f;
}

void mat4_mul(mat4_t *out, mat4_t *a, mat4_t *b) {
    mat4_t tmp;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp.m[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                tmp.m[i][j] += a->m[i][k] * b->m[k][j];
            }
        }
    }
    *out = tmp;
}

void mat4_translate(mat4_t *m, float x, float y, float z) {
    mat4_identity(m);
    m->m[0][3] = x;
    m->m[1][3] = y;
    m->m[2][3] = z;
}

void mat4_scale(mat4_t *m, float x, float y, float z) {
    mat4_identity(m);
    m->m[0][0] = x;
    m->m[1][1] = y;
    m->m[2][2] = z;
}

void mat4_rotate_x(mat4_t *m, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_identity(m);
    m->m[1][1] = c;  m->m[1][2] = -s;
    m->m[2][1] = s;  m->m[2][2] = c;
}

void mat4_rotate_y(mat4_t *m, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_identity(m);
    m->m[0][0] = c;  m->m[0][2] = s;
    m->m[2][0] = -s; m->m[2][2] = c;
}

void mat4_rotate_z(mat4_t *m, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_identity(m);
    m->m[0][0] = c;  m->m[0][1] = -s;
    m->m[1][0] = s;  m->m[1][1] = c;
}

void mat4_perspective(mat4_t *m, float fov, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov * 0.5f);
    float nf = 1.0f / (near - far);
    
    memset(m, 0, sizeof(mat4_t));
    m->m[0][0] = f / aspect;
    m->m[1][1] = f;
    m->m[2][2] = (far + near) * nf;
    m->m[2][3] = 2.0f * far * near * nf;
    m->m[3][2] = -1.0f;
}

void mat4_lookat(mat4_t *m, vec3_t *eye, vec3_t *center, vec3_t *up) {
    vec3_t f, s, u;
    
    f.x = center->x - eye->x;
    f.y = center->y - eye->y;
    f.z = center->z - eye->z;
    vec3_normalize(&f);
    
    vec3_cross(&s, &f, up);
    vec3_normalize(&s);
    
    vec3_cross(&u, &s, &f);
    
    memset(m, 0, sizeof(mat4_t));
    m->m[0][0] = s.x;  m->m[0][1] = s.y;  m->m[0][2] = s.z;
    m->m[1][0] = u.x;  m->m[1][1] = u.y;  m->m[1][2] = u.z;
    m->m[2][0] = -f.x; m->m[2][1] = -f.y; m->m[2][2] = -f.z;
    m->m[0][3] = -vec3_dot(&s, eye);
    m->m[1][3] = -vec3_dot(&u, eye);
    m->m[2][3] = vec3_dot(&f, eye);
    m->m[3][3] = 1.0f;
}

void mat4_transform_vec4(vec4_t *out, mat4_t *m, vec4_t *v) {
    out->x = m->m[0][0]*v->x + m->m[0][1]*v->y + m->m[0][2]*v->z + m->m[0][3]*v->w;
    out->y = m->m[1][0]*v->x + m->m[1][1]*v->y + m->m[1][2]*v->z + m->m[1][3]*v->w;
    out->z = m->m[2][0]*v->x + m->m[2][1]*v->y + m->m[2][2]*v->z + m->m[2][3]*v->w;
    out->w = m->m[3][0]*v->x + m->m[3][1]*v->y + m->m[3][2]*v->z + m->m[3][3]*v->w;
}

void mat4_transform_vec3(vec3_t *out, mat4_t *m, vec3_t *v) {
    vec4_t v4 = {v->x, v->y, v->z, 1.0f};
    vec4_t result;
    mat4_transform_vec4(&result, m, &v4);
    out->x = result.x;
    out->y = result.y;
    out->z = result.z;
}

// ============================================================================
// UPDATE MATRICES
// ============================================================================

static void gfx3d_update_mvp(void) {
    mat4_t tmp;
    mat4_mul(&tmp, &render_state.view, &render_state.model);
    mat4_mul(&render_state.mvp, &render_state.projection, &tmp);
    
    // Normal matrix (inverse transpose of model)
    render_state.normal_mat = render_state.model;
}

// ============================================================================
// PRESENT / FLIP
// ============================================================================

void gfx3d_present(uint32_t *screen_buffer, int screen_w, int screen_h) {
    framebuffer_t *fb = render_state.target;
    if (!fb || !screen_buffer) return;
    
    // Scale if needed
    if (fb->width == screen_w && fb->height == screen_h) {
        // Direct copy with SIMD
        simd_memcpy_fast(screen_buffer, fb->color, fb->width * fb->height * 4);
    } else {
        // Simple nearest-neighbor scaling
        float sx = (float)fb->width / screen_w;
        float sy = (float)fb->height / screen_h;
        
        for (int y = 0; y < screen_h; y++) {
            for (int x = 0; x < screen_w; x++) {
                int src_x = (int)(x * sx);
                int src_y = (int)(y * sy);
                screen_buffer[y * screen_w + x] = fb->color[src_y * fb->width + src_x];
            }
        }
    }
    
    render_state.frames_rendered++;
}

void gfx3d_flip(void) {
    // Swap buffers if double-buffered
}

void gfx3d_get_stats(uint64_t *tris, uint64_t *pixels, float *fps) {
    if (tris) *tris = render_state.triangles_drawn;
    if (pixels) *pixels = render_state.pixels_drawn;
    if (fps) *fps = render_state.fps;
}