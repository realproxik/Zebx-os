// ============================================================================
// ZEBX OS - GRAPHICS ENGINE (RTX 5090 Ready / Software Fallback)
// Billions of Pixels - AVX-512 Optimized
// ============================================================================

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"
#include "simd.h"

// Display modes
#define GFX_MODE_720P       0x01  // 1280x720
#define GFX_MODE_1080P      0x02  // 1920x1080
#define GFX_MODE_1440P      0x03  // 2560x1440
#define GFX_MODE_4K         0x04  // 3840x2160
#define GFX_MODE_8K         0x05  // 7680x4320

// Color formats
#define GFX_FMT_RGB888      0x00  // 24-bit
#define GFX_FMT_RGBA8888    0x01  // 32-bit with alpha
#define GFX_FMT_BGRA8888    0x02  // 32-bit BGRA
#define GFX_FMT_RGB565      0x03  // 16-bit

// Rendering pipeline stages
#define GFX_STAGE_VERTEX    0x01
#define GFX_STAGE_GEOMETRY  0x02
#define GFX_STAGE_RASTER    0x04
#define GFX_STAGE_FRAGMENT  0x08
#define GFX_STAGE_POSTFX    0x10

// Max limits
#define GFX_MAX_VERTICES    1000000
#define GFX_MAX_INDICES     3000000
#define GFX_MAX_TRIANGLES   500000
#define GFX_MAX_TEXTURES    1024
#define GFX_MAX_MESHES      4096
#define GFX_MAX_LIGHTS      256
#define GFX_MAX_POSTFX_PASSES 16

// Texture formats
#define TEX_FMT_RGBA8       0x00
#define TEX_FMT_RGB8        0x01
#define TEX_FMT_R8          0x02
#define TEX_FMT_RG8         0x03
#define TEX_FMT_RGBA16F     0x04
#define TEX_FMT_R32F        0x05
#define TEX_FMT_DEPTH24     0x06
#define TEX_FMT_DEPTH32F    0x07

// Blend modes
#define BLEND_NONE          0x00
#define BLEND_ALPHA         0x01
#define BLEND_ADD           0x02
#define BLEND_MULTIPLY      0x03
#define BLEND_SCREEN        0x04

// Cull modes
#define CULL_NONE           0x00
#define CULL_BACK           0x01
#define CULL_FRONT          0x02

// Depth test
#define DEPTH_NONE          0x00
#define DEPTH_LESS          0x01
#define DEPTH_EQUAL         0x02
#define DEPTH_LEQUAL        0x03
#define DEPTH_GREATER       0x04

// PostFX types
#define POSTFX_NONE         0x00
#define POSTFX_BLOOM        0x01
#define POSTFX_SSAO         0x02
#define POSTFX_FXAA         0x03
#define POSTFX_TAA          0x04
#define POSTFX_MOTION_BLUR  0x05
#define POSTFX_CHROMATIC    0x06
#define POSTFX_VIGNETTE     0x07
#define POSTFX_TONEMAP      0x08
#define POSTFX_DOF          0x09
#define POSTFX_LENS_FLARE   0x0A
#define POSTFX_COLOR_GRADE  0x0B
#define POSTFX_RAYTRACED_GI 0x0C  // RTX 5090 ready

// ============================================================================
// VECTOR / MATRIX TYPES
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
    vec4_t r[4];  // Row-major
} mat4_t;

// ============================================================================
// VERTEX
// ============================================================================

typedef struct {
    vec3_t position;
    vec3_t normal;
    vec2_t texcoord;
    vec4_t color;
    vec3_t tangent;
    vec3_t bitangent;
    vec4_t bone_weights;
    vec4_t bone_indices;
} vertex_t;

// ============================================================================
// MESH
// ============================================================================

typedef struct {
    char name[64];
    
    // Vertex data
    vertex_t *vertices;
    uint32_t num_vertices;
    uint32_t max_vertices;
    
    // Index data
    uint32_t *indices;
    uint32_t num_indices;
    uint32_t max_indices;
    
    // GPU buffers (simulated)
    uint32_t vbo_id;   // Vertex buffer object
    uint32_t ibo_id;   // Index buffer object
    
    // Bounding box
    vec3_t bbox_min;
    vec3_t bbox_max;
    vec3_t center;
    float radius;
    
    // Material
    uint32_t material_id;
    
    // Transform
    mat4_t transform;
    mat4_t normal_matrix;
    
    // Animation
    int animated;
    float anim_time;
    
    // LOD levels
    uint32_t lod_level;
    float lod_distance[4];
    
} mesh_t;

// ============================================================================
// TEXTURE
// ============================================================================

typedef struct {
    char name[64];
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t mip_levels;
    uint8_t *data[16];  // Mip chain
    uint32_t size[16];
    uint32_t id;
    int filtered;
    int wrapped;
    int compressed;
} texture_t;

// ============================================================================
// MATERIAL
// ============================================================================

typedef struct {
    char name[64];
    
    // Colors
    vec4_t albedo;
    vec4_t emissive;
    float metallic;
    float roughness;
    float ao;
    float specular;
    float opacity;
    
    // Textures
    uint32_t albedo_tex;
    uint32_t normal_tex;
    uint32_t metallic_tex;
    uint32_t roughness_tex;
    uint32_t ao_tex;
    uint32_t emissive_tex;
    uint32_t height_tex;
    
    // Shader
    uint32_t shader_id;
    
    // Flags
    int transparent;
    int double_sided;
    int unlit;
    
} material_t;

// ============================================================================
// LIGHT
// ============================================================================

typedef enum {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT,
    LIGHT_AREA,
    LIGHT_IBL,
} light_type_t;

typedef struct {
    light_type_t type;
    vec3_t position;
    vec3_t direction;
    vec3_t color;
    float intensity;
    float range;
    float spot_angle;
    float spot_softness;
    int cast_shadows;
    int enabled;
    uint32_t shadow_map_id;
} light_t;

// ============================================================================
// CAMERA
// ============================================================================

typedef struct {
    vec3_t position;
    vec3_t target;
    vec3_t up;
    vec3_t right;
    vec3_t forward;
    
    float fov;
    float aspect;
    float near_plane;
    float far_plane;
    
    mat4_t view;
    mat4_t projection;
    mat4_t view_projection;
    mat4_t inv_view;
    mat4_t inv_projection;
    
    // Movement
    float yaw;
    float pitch;
    float roll;
    float speed;
    
} camera_t;

// ============================================================================
// FRAMEBUFFER
// ============================================================================

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    
    // Color attachments
    uint32_t *color[8];
    uint32_t num_color;
    
    // Depth/Stencil
    float *depth;
    uint32_t *stencil;
    
    // HDR support
    float *hdr_color[4];
    int hdr_enabled;
    
    // MSAA
    uint32_t samples;
    
    // RTX 5090 features
    int raytracing_enabled;
    uint32_t rt_acceleration_structure;
    
} framebuffer_t;

// ============================================================================
// RASTERIZER STATE
// ============================================================================

typedef struct {
    uint32_t cull_mode;
    uint32_t depth_test;
    uint32_t depth_write;
    uint32_t blend_mode;
    int wireframe;
    int scissor_test;
    int scissor_x, scissor_y, scissor_w, scissor_h;
    int viewport_x, viewport_y, viewport_w, viewport_h;
} raster_state_t;

// ============================================================================
// POSTFX PASS
// ============================================================================

typedef struct {
    uint32_t type;
    int enabled;
    float intensity;
    void *params;  // Type-specific params
} postfx_pass_t;

typedef struct {
    float threshold;
    float intensity;
    float radius;
    uint32_t iterations;
    uint32_t mip_levels;
} bloom_params_t;

typedef struct {
    float radius;
    float bias;
    float intensity;
    uint32_t samples;
} ssao_params_t;

typedef struct {
    float exposure;
    float gamma;
    int auto_exposure;
    float min_luminance;
    float max_luminance;
    float adaptation_speed;
} tonemap_params_t;

// ============================================================================
// RENDERER STATE
// ============================================================================

typedef struct {
    // Display
    uint32_t display_width;
    uint32_t display_height;
    uint32_t display_mode;
    uint32_t display_format;
    uint32_t *framebuffer;
    uint32_t framebuffer_size;
    
    // Back buffers (double/triple buffering)
    uint32_t *back_buffer[3];
    uint32_t current_buffer;
    uint32_t num_buffers;
    
    // Depth buffer
    float *depth_buffer;
    uint32_t depth_buffer_size;
    
    // Meshes
    mesh_t meshes[GFX_MAX_MESHES];
    uint32_t num_meshes;
    
    // Textures
    texture_t textures[GFX_MAX_TEXTURES];
    uint32_t num_textures;
    
    // Materials
    material_t materials[1024];
    uint32_t num_materials;
    
    // Lights
    light_t lights[GFX_MAX_LIGHTS];
    uint32_t num_lights;
    
    // Camera
    camera_t camera;
    
    // Raster state
    raster_state_t raster;
    
    // PostFX
    postfx_pass_t postfx[GFX_MAX_POSTFX_PASSES];
    uint32_t num_postfx;
    
    // Stats
    uint64_t triangles_drawn;
    uint64_t pixels_drawn;
    uint64_t draw_calls;
    uint64_t frame_count;
    float fps;
    float frame_time_ms;
    float delta_time;
    
    // SIMD
    int avx512_available;
    int avx2_available;
    int sse4_available;
    
    // RTX 5090
    int nvidia_detected;
    uint32_t nvidia_device_id;
    uint32_t nvidia_vram_mb;
    int cuda_available;
    int rt_cores_available;
    int tensor_cores_available;
    
    // Settings
    int vsync;
    int fullscreen;
    int hdr_enabled;
    int msaa_samples;
    int anisotropic;
    
} gfx_state_t;

// ============================================================================
// FUNCTIONS
// ============================================================================

// Init
void gfx_init(uint32_t width, uint32_t height, uint32_t mode);
void gfx_shutdown(void);
gfx_state_t* gfx_get_state(void);

// Display
void gfx_set_mode(uint32_t mode);
void gfx_set_resolution(uint32_t width, uint32_t height);
void gfx_present(void);
void gfx_clear(uint32_t color);
void gfx_clear_depth(float depth);
void gfx_swap_buffers(void);

// Mesh
mesh_t* gfx_mesh_create(const char *name, uint32_t max_vertices, uint32_t max_indices);
void gfx_mesh_destroy(mesh_t *mesh);
void gfx_mesh_add_vertex(mesh_t *mesh, vertex_t *v);
void gfx_mesh_add_triangle(mesh_t *mesh, uint32_t i0, uint32_t i1, uint32_t i2);
void gfx_mesh_update_bounds(mesh_t *mesh);
void gfx_mesh_transform(mesh_t *mesh, mat4_t *transform);
void gfx_mesh_draw(mesh_t *mesh);
mesh_t* gfx_mesh_load_obj(const char *path);
mesh_t* gfx_mesh_create_cube(float size);
mesh_t* gfx_mesh_create_sphere(float radius, uint32_t segments);
mesh_t* gfx_mesh_create_plane(float width, float depth, uint32_t subdivs);
mesh_t* gfx_mesh_create_cylinder(float radius, float height, uint32_t segments);
mesh_t* gfx_mesh_create_torus(float major_radius, float minor_radius, uint32_t major_seg, uint32_t minor_seg);

// Texture
texture_t* gfx_texture_create(const char *name, uint32_t width, uint32_t height, uint32_t format);
void gfx_texture_destroy(texture_t *tex);
void gfx_texture_upload(texture_t *tex, uint8_t *data);
void gfx_texture_generate_mipmaps(texture_t *tex);
texture_t* gfx_texture_load_bmp(const char *path);
texture_t* gfx_texture_load_png(const char *path);
texture_t* gfx_texture_create_checkerboard(uint32_t size, uint32_t c1, uint32_t c2);

// Material
material_t* gfx_material_create(const char *name);
void gfx_material_set_texture(material_t *mat, uint32_t slot, uint32_t tex_id);

// Light
light_t* gfx_light_create(light_type_t type);
void gfx_light_set_position(light_t *light, float x, float y, float z);
void gfx_light_set_color(light_t *light, float r, float g, float b);
void gfx_light_set_intensity(light_t *light, float intensity);

// Camera
void gfx_camera_init(camera_t *cam, float fov, float aspect, float near, float far);
void gfx_camera_look_at(camera_t *cam, float eye_x, float eye_y, float eye_z, 
                        float target_x, float target_y, float target_z);
void gfx_camera_update(camera_t *cam);
void gfx_camera_move_forward(camera_t *cam, float dist);
void gfx_camera_move_right(camera_t *cam, float dist);
void gfx_camera_move_up(camera_t *cam, float dist);
void gfx_camera_rotate(camera_t *cam, float yaw, float pitch);

// Rasterizer
void gfx_raster_set_state(raster_state_t *state);
void gfx_raster_draw_triangle(vertex_t *v0, vertex_t *v1, vertex_t *v2);
void gfx_raster_draw_triangle_list(mesh_t *mesh);
void gfx_raster_draw_indexed(mesh_t *mesh);
void gfx_raster_line(float x0, float y0, float x1, float y1, uint32_t color);
void gfx_raster_rect(int x, int y, int w, int h, uint32_t color);
void gfx_raster_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_raster_circle(int cx, int cy, int r, uint32_t color);
void gfx_raster_fill_circle(int cx, int cy, int r, uint32_t color);
void gfx_raster_triangle_2d(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void gfx_raster_textured_triangle(vertex_t *v0, vertex_t *v1, vertex_t *v2, texture_t *tex);

// SIMD-optimized rasterization
void gfx_raster_triangle_avx512(vertex_t *v0, vertex_t *v1, vertex_t *v2, uint32_t color);
void gfx_raster_triangle_avx2(vertex_t *v0, vertex_t *v1, vertex_t *v2, uint32_t color);
void gfx_raster_triangle_sse4(vertex_t *v0, vertex_t *v1, vertex_t *v2, uint32_t color);
void gfx_raster_batch_avx512(mesh_t *mesh, uint32_t color);

// PostFX
void gfx_postfx_add(uint32_t type, float intensity);
void gfx_postfx_remove(uint32_t index);
void gfx_postfx_enable(uint32_t index);
void gfx_postfx_disable(uint32_t index);
void gfx_postfx_set_intensity(uint32_t index, float intensity);
void gfx_postfx_apply(framebuffer_t *fb);

// Individual postFX
void gfx_postfx_bloom(framebuffer_t *fb, bloom_params_t *params);
void gfx_postfx_ssao(framebuffer_t *fb, ssao_params_t *params);
void gfx_postfx_fxaa(framebuffer_t *fb);
void gfx_postfx_tonemap(framebuffer_t *fb, tonemap_params_t *params);
void gfx_postfx_chromatic_aberration(framebuffer_t *fb, float intensity);
void gfx_postfx_vignette(framebuffer_t *fb, float intensity);
void gfx_postfx_motion_blur(framebuffer_t *fb, float intensity);
void gfx_postfx_dof(framebuffer_t *fb, float focus_distance, float aperture);
void gfx_postfx_color_grade(framebuffer_t *fb, float *lut_3d);

// NVIDIA RTX 5090 specific
void gfx_rtx_init(void);
void gfx_rtx_create_acceleration_structure(mesh_t *mesh);
void gfx_rtx_trace_rays(camera_t *cam, framebuffer_t *fb);
void gfx_rtx_denoise(framebuffer_t *fb);
void gfx_rtx_dlss_upsample(framebuffer_t *fb, uint32_t target_width, uint32_t target_height);

// Math utilities
void mat4_identity(mat4_t *m);
void mat4_multiply(mat4_t *a, mat4_t *b, mat4_t *out);
void mat4_translate(mat4_t *m, float x, float y, float z);
void mat4_rotate_x(mat4_t *m, float angle);
void mat4_rotate_y(mat4_t *m, float angle);
void mat4_rotate_z(mat4_t *m, float angle);
void mat4_scale(mat4_t *m, float x, float y, float z);
void mat4_perspective(mat4_t *m, float fov, float aspect, float near, float far);
void mat4_look_at(mat4_t *m, vec3_t *eye, vec3_t *target, vec3_t *up);
void mat4_inverse(mat4_t *m, mat4_t *out);
void mat4_transpose(mat4_t *m, mat4_t *out);
vec4_t mat4_multiply_vec4(mat4_t *m, vec4_t *v);

vec3_t vec3_add(vec3_t *a, vec3_t *b);
vec3_t vec3_sub(vec3_t *a, vec3_t *b);
vec3_t vec3_mul(vec3_t *a, float s);
vec3_t vec3_cross(vec3_t *a, vec3_t *b);
float vec3_dot(vec3_t *a, vec3_t *b);
float vec3_length(vec3_t *a);
vec3_t vec3_normalize(vec3_t *a);

// Debug
void gfx_debug_draw_line(vec3_t *start, vec3_t *end, uint32_t color);
void gfx_debug_draw_bbox(vec3_t *min, vec3_t *max, uint32_t color);
void gfx_debug_draw_normals(mesh_t *mesh, float length);
void gfx_stats_reset(void);
void gfx_stats_print(void);

#endif