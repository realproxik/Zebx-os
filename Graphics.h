#ifndef GRAPHICS3D_H
#define GRAPHICS3D_H

#include "types.h"
#include "simd.h"

// ============================================================================
// ZEBX OS - 3D GRAPHICS ENGINE (Software Rasterizer)
// AVX-512 Optimized | Billions of Pixels | Gaming OS
// ============================================================================

#define GFX_MAX_TRIANGLES       1000000
#define GFX_MAX_VERTICES        500000
#define GFX_MAX_TEXTURES        256
#define GFX_MAX_LIGHTS          16
#define GFX_MAX_FRAMEBUFFERS    4

#define GFX_BACKBUFFER_WIDTH    1920
#define GFX_BACKBUFFER_HEIGHT   1080
#define GFX_BACKBUFFER_SIZE     (GFX_BACKBUFFER_WIDTH * GFX_BACKBUFFER_HEIGHT)

// Color formats
#define GFX_FMT_RGBA8888        0
#define GFX_FMT_RGB565          1
#define GFX_FMT_BGRA8888        2

// Render flags
#define GFX_RENDER_WIREFRAME    0x01
#define GFX_RENDER_FLAT         0x02
#define GFX_RENDER_GOURAUD      0x04
#define GFX_RENDER_TEXTURED     0x08
#define GFX_RENDER_ZBUFFER      0x10
#define GFX_RENDER_CULL_BACK    0x20
#define GFX_RENDER_CULL_FRONT   0x40
#define GFX_RENDER_FOG          0x80
#define GFX_RENDER_SHADOWS      0x100
#define GFX_RENDER_BLOOM        0x200
#define GFX_RENDER_MOTION_BLUR  0x400
#define GFX_RENDER_SSAO         0x800

// ============================================================================
// MATH TYPES (3D)
// ============================================================================

typedef struct {
    float x, y, z, w;
} vec4_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x, y;
} vec2_t;

typedef struct {
    float m[4][4];
} mat4_t;

// ============================================================================
// VERTEX & TRIANGLE
// ============================================================================

typedef struct {
    vec3_t pos;      // Position
    vec3_t normal;   // Normal
    vec2_t uv;       // Texture coordinates
    uint32_t color;  // Vertex color (RGBA)
} vertex_t;

typedef struct {
    uint32_t v[3];   // Vertex indices
    uint32_t tex_id; // Texture index
    uint32_t flags;  // Triangle flags
} triangle_t;

// ============================================================================
// LIGHTING
// ============================================================================

typedef enum {
    LIGHT_AMBIENT,
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT,
} light_type_t;

typedef struct {
    light_type_t type;
    vec3_t pos;
    vec3_t dir;
    vec3_t color;
    float intensity;
    float range;
    float spot_angle;
    float spot_penumbra;
} light_t;

// ============================================================================
// TEXTURE
// ============================================================================

typedef struct {
    uint32_t *data;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t mip_levels;
    uint32_t *mipmaps[12]; // Up to 4096x4096
} texture_t;

// ============================================================================
// FRAMEBUFFER
// ============================================================================

typedef struct {
    uint32_t *color;    // RGBA8888 color buffer
    float *depth;       // Z-buffer (32-bit float)
    uint32_t *stencil;  // Stencil buffer
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} framebuffer_t;

// ============================================================================
// RENDER STATE
// ============================================================================

typedef struct {
    // Matrices
    mat4_t model;
    mat4_t view;
    mat4_t projection;
    mat4_t mvp;          // Model-View-Projection
    mat4_t normal_mat;   // Normal matrix
    
    // Viewport
    int vp_x, vp_y;
    int vp_w, vp_h;
    
    // Render settings
    uint32_t flags;
    uint32_t clear_color;
    float clear_depth;
    uint32_t clear_stencil;
    
    // Lighting
    light_t lights[GFX_MAX_LIGHTS];
    uint32_t num_lights;
    vec3_t ambient_color;
    float ambient_intensity;
    
    // Fog
    float fog_start;
    float fog_end;
    vec3_t fog_color;
    
    // Current framebuffer
    framebuffer_t *target;
    
    // Statistics
    uint64_t triangles_drawn;
    uint64_t pixels_drawn;
    uint64_t frames_rendered;
    float fps;
    float frame_time_ms;
    
} render_state_t;

// ============================================================================
// FUNCTIONS
// ============================================================================

// Init
void gfx3d_init(void);
void gfx3d_shutdown(void);
render_state_t* gfx3d_get_state(void);

// Framebuffer
framebuffer_t* gfx3d_create_framebuffer(uint32_t w, uint32_t h);
void gfx3d_destroy_framebuffer(framebuffer_t *fb);
void gfx3d_set_framebuffer(framebuffer_t *fb);
void gfx3d_clear(uint32_t color, float depth, uint32_t stencil);

// Math
void vec3_set(vec3_t *v, float x, float y, float z);
void vec3_add(vec3_t *out, vec3_t *a, vec3_t *b);
void vec3_sub(vec3_t *out, vec3_t *a, vec3_t *b);
void vec3_mul(vec3_t *out, vec3_t *a, float s);
float vec3_dot(vec3_t *a, vec3_t *b);
void vec3_cross(vec3_t *out, vec3_t *a, vec3_t *b);
float vec3_len(vec3_t *v);
void vec3_normalize(vec3_t *v);

void mat4_identity(mat4_t *m);
void mat4_mul(mat4_t *out, mat4_t *a, mat4_t *b);
void mat4_translate(mat4_t *m, float x, float y, float z);
void mat4_scale(mat4_t *m, float x, float y, float z);
void mat4_rotate_x(mat4_t *m, float angle);
void mat4_rotate_y(mat4_t *m, float angle);
void mat4_rotate_z(mat4_t *m, float angle);
void mat4_perspective(mat4_t *m, float fov, float aspect, float near, float far);
void mat4_lookat(mat4_t *m, vec3_t *eye, vec3_t *center, vec3_t *up);
void mat4_transform_vec4(vec4_t *out, mat4_t *m, vec4_t *v);
void mat4_transform_vec3(vec3_t *out, mat4_t *m, vec3_t *v);

// Mesh
void gfx3d_load_mesh(vertex_t *verts, uint32_t num_verts, triangle_t *tris, uint32_t num_tris);
void gfx3d_draw_mesh(uint32_t mesh_id, mat4_t *transform);

// Immediate mode (legacy)
void gfx3d_begin(uint32_t primitive);
void gfx3d_vertex(float x, float y, float z);
void gfx3d_texcoord(float u, float v);
void gfx3d_normal(float x, float y, float z);
void gfx3d_color(uint32_t rgba);
void gfx3d_end(void);

// Lighting
void gfx3d_set_light(uint32_t idx, light_t *light);
void gfx3d_set_ambient(vec3_t *color, float intensity);

// Textures
texture_t* gfx3d_create_texture(uint32_t w, uint32_t h, uint32_t format);
void gfx3d_destroy_texture(texture_t *tex);
void gfx3d_load_texture(texture_t *tex, uint32_t *data);
void gfx3d_bind_texture(uint32_t slot, texture_t *tex);
void gfx3d_generate_mipmaps(texture_t *tex);

// Rasterization (SIMD optimized)
void gfx3d_rasterize_triangle(vertex_t *v0, vertex_t *v1, vertex_t *v2);
void gfx3d_rasterize_triangle_avx512(vertex_t *v0, vertex_t *v1, vertex_t *v2);
void gfx3d_rasterize_triangle_avx2(vertex_t *v0, vertex_t *v1, vertex_t *v2);
void gfx3d_rasterize_triangle_sse(vertex_t *v0, vertex_t *v1, vertex_t *v2);

// Post-processing
void gfx3d_apply_bloom(framebuffer_t *fb, float intensity);
void gfx3d_apply_ssao(framebuffer_t *fb, float radius, float intensity);
void gfx3d_apply_motion_blur(framebuffer_t *fb, float strength);
void gfx3d_apply_tonemap(framebuffer_t *fb, float exposure);

// Present
void gfx3d_present(uint32_t *screen_buffer, int screen_w, int screen_h);
void gfx3d_flip(void);

// Stats
void gfx3d_get_stats(uint64_t *tris, uint64_t *pixels, float *fps);

#endif