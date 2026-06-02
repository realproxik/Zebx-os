// ============================================================================
// ZEBX OS - 3D MESH SYSTEM
// Part 3: Mesh Loading, Transform, Draw
// ============================================================================

#include "graphics3d.h"

static uint32_t next_mesh_id = 1;

typedef struct {
    uint32_t id;
    vertex_t *vertices;
    triangle_t *triangles;
    uint32_t num_vertices;
    uint32_t num_triangles;
    uint32_t flags;
} mesh_t;

static mesh_t meshes[1024];
static uint32_t num_meshes = 0;

// ============================================================================
// PRIMITIVE MESHES
// ============================================================================

void gfx3d_create_cube(float size, uint32_t color) {
    float s = size * 0.5f;
    uint32_t base_v = num_vertices;
    
    // 8 vertices
    vertex_t cube_verts[8] = {
        {{-s, -s, -s}, {-1,-1,-1}, {0,0}, color},
        {{ s, -s, -s}, { 1,-1,-1}, {1,0}, color},
        {{ s,  s, -s}, { 1, 1,-1}, {1,1}, color},
        {{-s,  s, -s}, {-1, 1,-1}, {0,1}, color},
        {{-s, -s,  s}, {-1,-1, 1}, {0,0}, color},
        {{ s, -s,  s}, { 1,-1, 1}, {1,0}, color},
        {{ s,  s,  s}, { 1, 1, 1}, {1,1}, color},
        {{-s,  s,  s}, {-1, 1, 1}, {0,1}, color},
    };
    
    for (int i = 0; i < 8; i++) {
        vertex_buffer[num_vertices++] = cube_verts[i];
    }
    
    // 12 triangles (2 per face)
    triangle_t cube_tris[12] = {
        {0,1,2,0,0}, {0,2,3,0,0}, // Front
        {4,6,5,0,0}, {4,7,6,0,0}, // Back
        {0,4,5,0,0}, {0,5,1,0,0}, // Bottom
        {2,6,7,0,0}, {2,7,3,0,0}, // Top
        {0,3,7,0,0}, {0,7,4,0,0}, // Left
        {1,5,6,0,0}, {1,6,2,0,0}, // Right
    };
    
    for (int i = 0; i < 12; i++) {
        cube_tris[i].v[0] += base_v;
        cube_tris[i].v[1] += base_v;
        cube_tris[i].v[2] += base_v;
        triangle_buffer[num_triangles++] = cube_tris[i];
    }
}

void gfx3d_create_sphere(float radius, uint32_t segments, uint32_t rings, uint32_t color) {
    uint32_t base_v = num_vertices;
    
    // Top pole
    vertex_t top = {{0, radius, 0}, {0,1,0}, {0.5f, 0}, color};
    vertex_buffer[num_vertices++] = top;
    
    // Rings
    for (uint32_t r = 1; r < rings; r++) {
        float phi = 3.14159f * r / rings;
        float y = radius * cosf(phi);
        float ring_radius = radius * sinf(phi);
        
        for (uint32_t s = 0; s < segments; s++) {
            float theta = 2.0f * 3.14159f * s / segments;
            float x = ring_radius * cosf(theta);
            float z = ring_radius * sinf(theta);
            
            vec3_t normal = {x, y, z};
            vec3_normalize(&normal);
            
            vertex_t v = {{x, y, z}, normal, {(float)s/segments, (float)r/rings}, color};
            vertex_buffer[num_vertices++] = v;
        }
    }
    
    // Bottom pole
    vertex_t bottom = {{0, -radius, 0}, {0,-1,0}, {0.5f, 1}, color};
    vertex_buffer[num_vertices++] = bottom;
    
    // Generate triangles
    uint32_t top_idx = base_v;
    uint32_t bottom_idx = num_vertices - 1;
    uint32_t first_ring = base_v + 1;
    
    // Top cap
    for (uint32_t s = 0; s < segments; s++) {
        uint32_t next_s = (s + 1) % segments;
        triangle_t t = {top_idx, first_ring + s, first_ring + next_s, 0, 0};
        triangle_buffer[num_triangles++] = t;
    }
    
    // Middle rings
    for (uint32_t r = 0; r < rings - 2; r++) {
        uint32_t ring_start = first_ring + r * segments;
        uint32_t next_ring = ring_start + segments;
        
        for (uint32_t s = 0; s < segments; s++) {
            uint32_t next_s = (s + 1) % segments;
            
            triangle_t t1 = {ring_start + s, next_ring + s, next_ring + next_s, 0, 0};
            triangle_t t2 = {ring_start + s, next_ring + next_s, ring_start + next_s, 0, 0};
            
            triangle_buffer[num_triangles++] = t1;
            triangle_buffer[num_triangles++] = t2;
        }
    }
    
    // Bottom cap
    uint32_t last_ring = first_ring + (rings - 2) * segments;
    for (uint32_t s = 0; s < segments; s++) {
        uint32_t next_s = (s + 1) % segments;
        triangle_t t = {last_ring + s, bottom_idx, last_ring + next_s, 0, 0};
        triangle_buffer[num_triangles++] = t;
    }
}

void gfx3d_create_plane(float width, float depth, uint32_t color) {
    uint32_t base_v = num_vertices;
    float w = width * 0.5f;
    float d = depth * 0.5f;
    
    vertex_t verts[4] = {
        {{-w, 0, -d}, {0,1,0}, {0,0}, color},
        {{ w, 0, -d}, {0,1,0}, {1,0}, color},
        {{ w, 0,  d}, {0,1,0}, {1,1}, color},
        {{-w, 0,  d}, {0,1,0}, {0,1}, color},
    };
    
    for (int i = 0; i < 4; i++) {
        vertex_buffer[num_vertices++] = verts[i];
    }
    
    triangle_t tris[2] = {
        {base_v, base_v+1, base_v+2, 0, 0},
        {base_v, base_v+2, base_v+3, 0, 0},
    };
    
    for (int i = 0; i < 2; i++) {
        triangle_buffer[num_triangles++] = tris[i];
    }
}

// ============================================================================
// DRAW MESH
// ============================================================================

void gfx3d_draw_mesh(uint32_t mesh_id, mat4_t *transform) {
    if (!transform) return;
    
    gfx3d_update_mvp();
    
    // Transform all vertices to clip space
    static vec4_t clip_verts[GFX_MAX_VERTICES];
    
    for (uint32_t i = 0; i < num_vertices; i++) {
        vec4_t v = {vertex_buffer[i].pos.x, vertex_buffer[i].pos.y, vertex_buffer[i].pos.z, 1.0f};
        mat4_transform_vec4(&clip_verts[i], &render_state.mvp, &v);
        
        // Perspective divide
        if (clip_verts[i].w != 0) {
            clip_verts[i].x /= clip_verts[i].w;
            clip_verts[i].y /= clip_verts[i].w;
            clip_verts[i].z /= clip_verts[i].w;
        }
        
        // Viewport transform
        clip_verts[i].x = (clip_verts[i].x + 1.0f) * 0.5f * render_state.vp_w + render_state.vp_x;
        clip_verts[i].y = (1.0f - clip_verts[i].y) * 0.5f * render_state.vp_h + render_state.vp_y;
    }
    
    // Rasterize triangles
    for (uint32_t i = 0; i < num_triangles; i++) {
        triangle_t *tri = &triangle_buffer[i];
        
        // Backface culling
        if (render_state.flags & GFX_RENDER_CULL_BACK) {
            vec3_t e0, e1, normal;
            vec3_sub(&e0, &vertex_buffer[tri->v[1]].pos, &vertex_buffer[tri->v[0]].pos);
            vec3_sub(&e1, &vertex_buffer[tri->v[2]].pos, &vertex_buffer[tri->v[0]].pos);
            vec3_cross(&normal, &e0, &e1);
            if (normal.z > 0) continue; // Back-facing
        }
        
        // Create transformed vertices for rasterizer
        vertex_t tv0 = vertex_buffer[tri->v[0]];
        vertex_t tv1 = vertex_buffer[tri->v[1]];
        vertex_t tv2 = vertex_buffer[tri->v[2]];
        
        tv0.pos.x = clip_verts[tri->v[0]].x;
        tv0.pos.y = clip_verts[tri->v[0]].y;
        tv0.pos.z = clip_verts[tri->v[0]].z;
        
        tv1.pos.x = clip_verts[tri->v[1]].x;
        tv1.pos.y = clip_verts[tri->v[1]].y;
        tv1.pos.z = clip_verts[tri->v[1]].z;
        
        tv2.pos.x = clip_verts[tri->v[2]].x;
        tv2.pos.y = clip_verts[tri->v[2]].y;
        tv2.pos.z = clip_verts[tri->v[2]].z;
        
        // Choose rasterizer based on SIMD support
        if (simd_has_feature(SIMD_FEATURE_AVX512F)) {
            gfx3d_rasterize_triangle_avx512(&tv0, &tv1, &tv2);
        } else if (simd_has_feature(SIMD_FEATURE_AVX2)) {
            gfx3d_rasterize_triangle_avx2(&tv0, &tv1, &tv2);
        } else if (simd_has_feature(SIMD_FEATURE_SSE42)) {
            gfx3d_rasterize_triangle_sse(&tv0, &tv1, &tv2);
        } else {
            gfx3d_rasterize_triangle(&tv0, &tv1, &tv2);
        }
    }
}

// ============================================================================
// DEMO SCENE
// ============================================================================

void gfx3d_demo_scene(void) {
    // Clear
    gfx3d_clear(render_state.clear_color, 1.0f, 0);
    
    // Camera
    vec3_t eye = {0, 2, 5};
    vec3_t center = {0, 0, 0};
    vec3_t up = {0, 1, 0};
    mat4_lookat(&render_state.view, &eye, &center, &up);
    
    // Projection
    mat4_perspective(&render_state.projection, 1.047f, 16.0f/9.0f, 0.1f, 100.0f);
    
    // Reset mesh buffers
    num_vertices = 0;
    num_triangles = 0;
    
    // Create scene
    static float rot = 0;
    rot += 0.02f;
    
    // Cube 1
    mat4_t model;
    mat4_identity(&model);
    mat4_translate(&model, -1.5f, 0, 0);
    mat4_rotate_y(&model, rot);
    mat4_scale(&model, 1, 1, 1);
    render_state.model = model;
    
    gfx3d_create_cube(1.0f, 0xFFFF4444); // Red cube
    
    // Cube 2
    mat4_translate(&model, 1.5f, 0, 0);
    mat4_rotate_x(&model, rot * 0.7f);
    render_state.model = model;
    
    gfx3d_create_cube(1.0f, 0xFF44FF44); // Green cube
    
    // Floor
    mat4_identity(&model);
    mat4_translate(&model, 0, -1, 0);
    mat4_scale(&model, 5, 0.1f, 5);
    render_state.model = model;
    
    gfx3d_create_cube(1.0f, 0xFF4444FF); // Blue floor
    
    // Draw everything
    gfx3d_draw_mesh(0, &render_state.mvp);
}