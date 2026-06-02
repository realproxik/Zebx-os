// ============================================================================
// ZEBX OS - 3D RASTERIZER
// Part 2: Triangle Rasterization (SIMD Optimized)
// ============================================================================

#include "graphics3d.h"
#include "simd.h"

// ============================================================================
// EDGE FUNCTION (for barycentric coordinates)
// ============================================================================

static inline float edge_function(vec3_t *a, vec3_t *b, vec3_t *c) {
    return (c->x - a->x) * (b->y - a->y) - (c->y - a->y) * (b->x - a->x);
}

// ============================================================================
// PIXEL SHADER (simplified)
// ============================================================================

static inline uint32_t shade_pixel(vertex_t *v0, vertex_t *v1, vertex_t *v2,
                                  float w0, float w1, float w2,
                                  vec3_t *normal, uint32_t flags) {
    // Interpolate color
    uint32_t c0 = v0->color;
    uint32_t c1 = v1->color;
    uint32_t c2 = v2->color;
    
    float r = ((c0 >> 16) & 0xFF) * w0 + ((c1 >> 16) & 0xFF) * w1 + ((c2 >> 16) & 0xFF) * w2;
    float g = ((c0 >> 8) & 0xFF) * w0 + ((c1 >> 8) & 0xFF) * w1 + ((c2 >> 8) & 0xFF) * w2;
    float b = (c0 & 0xFF) * w0 + (c1 & 0xFF) * w1 + (c2 & 0xFF) * w2;
    
    // Simple lighting
    if (flags & (GFX_RENDER_FLAT | GFX_RENDER_GOURAUD)) {
        vec3_t light_dir = {0.0f, 0.0f, -1.0f};  // Light from camera
        float diff = vec3_dot(normal, &light_dir);
        if (diff < 0) diff = 0;
        if (diff > 1) diff = 1;
        
        float ambient = 0.2f;
        float intensity = ambient + diff * (1.0f - ambient);
        
        r *= intensity;
        g *= intensity;
        b *= intensity;
    }
    
    // Clamp
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// ============================================================================
// SCALAR TRIANGLE RASTERIZER
// ============================================================================

void gfx3d_rasterize_triangle(vertex_t *v0, vertex_t *v1, vertex_t *v2) {
    framebuffer_t *fb = render_state.target;
    if (!fb) return;
    
    // Compute bounding box
    float min_x = v0->pos.x;
    float min_y = v0->pos.y;
    float max_x = v0->pos.x;
    float max_y = v0->pos.y;
    
    if (v1->pos.x < min_x) min_x = v1->pos.x;
    if (v1->pos.x > max_x) max_x = v1->pos.x;
    if (v1->pos.y < min_y) min_y = v1->pos.y;
    if (v1->pos.y > max_y) max_y = v1->pos.y;
    
    if (v2->pos.x < min_x) min_x = v2->pos.x;
    if (v2->pos.x > max_x) max_x = v2->pos.x;
    if (v2->pos.y < min_y) min_y = v2->pos.y;
    if (v2->pos.y > max_y) max_y = v2->pos.y;
    
    // Clip to viewport
    int x0 = (int)min_x;
    int y0 = (int)min_y;
    int x1 = (int)(max_x + 1);
    int y1 = (int)(max_y + 1);
    
    if (x0 < render_state.vp_x) x0 = render_state.vp_x;
    if (y0 < render_state.vp_y) y0 = render_state.vp_y;
    if (x1 > render_state.vp_x + render_state.vp_w) x1 = render_state.vp_x + render_state.vp_w;
    if (y1 > render_state.vp_y + render_state.vp_h) y1 = render_state.vp_y + render_state.vp_h;
    
    // Edge function values
    float area = edge_function(&v0->pos, &v1->pos, &v2->pos);
    if (area <= 0) return; // Back-facing or degenerate
    
    float inv_area = 1.0f / area;
    
    // Compute normal for flat shading
    vec3_t normal;
    vec3_t e0, e1;
    vec3_sub(&e0, &v1->pos, &v0->pos);
    vec3_sub(&e1, &v2->pos, &v0->pos);
    vec3_cross(&normal, &e0, &e1);
    vec3_normalize(&normal);
    
    // Rasterize
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            vec3_t p = {(float)x + 0.5f, (float)y + 0.5f, 0.0f};
            
            float w0 = edge_function(&v1->pos, &v2->pos, &p);
            float w1 = edge_function(&v2->pos, &v0->pos, &p);
            float w2 = edge_function(&v0->pos, &v1->pos, &p);
            
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                w0 *= inv_area;
                w1 *= inv_area;
                w2 *= inv_area;
                
                // Interpolate Z
                float z = v0->pos.z * w0 + v1->pos.z * w1 + v2->pos.z * w2;
                
                // Z-test
                uint32_t idx = y * fb->width + x;
                if (render_state.flags & GFX_RENDER_ZBUFFER) {
                    if (z < fb->depth[idx]) {
                        fb->depth[idx] = z;
                        fb->color[idx] = shade_pixel(v0, v1, v2, w0, w1, w2, &normal, render_state.flags);
                        render_state.pixels_drawn++;
                    }
                } else {
                    fb->color[idx] = shade_pixel(v0, v1, v2, w0, w1, w2, &normal, render_state.flags);
                    render_state.pixels_drawn++;
                }
            }
        }
    }
    
    render_state.triangles_drawn++;
}

// ============================================================================
// AVX-512 OPTIMIZED RASTERIZER (16 pixels at once!)
// ============================================================================

void gfx3d_rasterize_triangle_avx512(vertex_t *v0, vertex_t *v1, vertex_t *v2) {
    if (!simd_has_feature(SIMD_FEATURE_AVX512F)) {
        gfx3d_rasterize_triangle(v0, v1, v2);
        return;
    }
    
    framebuffer_t *fb = render_state.target;
    if (!fb) return;
    
    // Compute bounding box (same as scalar)
    float min_x = v0->pos.x, min_y = v0->pos.y;
    float max_x = v0->pos.x, max_y = v0->pos.y;
    
    if (v1->pos.x < min_x) min_x = v1->pos.x;
    if (v1->pos.x > max_x) max_x = v1->pos.x;
    if (v1->pos.y < min_y) min_y = v1->pos.y;
    if (v1->pos.y > max_y) max_y = v1->pos.y;
    if (v2->pos.x < min_x) min_x = v2->pos.x;
    if (v2->pos.x > max_x) max_x = v2->pos.x;
    if (v2->pos.y < min_y) min_y = v2->pos.y;
    if (v2->pos.y > max_y) max_y = v2->pos.y;
    
    int x0 = (int)min_x;
    int y0 = (int)min_y;
    int x1 = (int)(max_x + 1);
    int y1 = (int)(max_y + 1);
    
    if (x0 < render_state.vp_x) x0 = render_state.vp_x;
    if (y0 < render_state.vp_y) y0 = render_state.vp_y;
    if (x1 > render_state.vp_x + render_state.vp_w) x1 = render_state.vp_x + render_state.vp_w;
    if (y1 > render_state.vp_y + render_state.vp_h) y1 = render_state.vp_y + render_state.vp_h;
    
    // Align x0 to 16-pixel boundary for AVX-512
    int x0_aligned = x0 & ~15;
    
    float area = edge_function(&v0->pos, &v1->pos, &v2->pos);
    if (area <= 0) return;
    float inv_area = 1.0f / area;
    
    // Precompute edge function coefficients
    float e0_a = v1->pos.y - v2->pos.y;
    float e0_b = v2->pos.x - v1->pos.x;
    float e0_c = v1->pos.x * v2->pos.y - v2->pos.x * v1->pos.y;
    
    float e1_a = v2->pos.y - v0->pos.y;
    float e1_b = v0->pos.x - v2->pos.x;
    float e1_c = v2->pos.x * v0->pos.y - v0->pos.x * v2->pos.y;
    
    float e2_a = v0->pos.y - v1->pos.y;
    float e2_b = v1->pos.x - v0->pos.x;
    float e2_c = v0->pos.x * v1->pos.y - v1->pos.x * v0->pos.y;
    
    // Normal for flat shading
    vec3_t normal;
    vec3_t e0, e1;
    vec3_sub(&e0, &v1->pos, &v0->pos);
    vec3_sub(&e1, &v2->pos, &v0->pos);
    vec3_cross(&normal, &e0, &e1);
    vec3_normalize(&normal);
    
    // AVX-512 constants
    __m512 zero = _mm512_set1_ps(0.0f);
    __m512 one = _mm512_set1_ps(1.0f);
    __m512 inv_area_vec = _mm512_set1_ps(inv_area);
    
    // Pixel offset pattern: 0.5, 1.5, 2.5, ..., 15.5
    __m512 px_offset = _mm512_set_ps(15.5f, 14.5f, 13.5f, 12.5f, 11.5f, 10.5f, 9.5f, 8.5f,
                                      7.5f, 6.5f, 5.5f, 4.5f, 3.5f, 2.5f, 1.5f, 0.5f);
    
    for (int y = y0; y < y1; y++) {
        float py = (float)y + 0.5f;
        
        for (int x = x0_aligned; x < x1; x += 16) {
            float px_base = (float)x;
            __m512 px = _mm512_add_ps(_mm512_set1_ps(px_base), px_offset);
            __m512 py_vec = _mm512_set1_ps(py);
            
            // Edge 0: w0 = e0_a * px + e0_b * py + e0_c
            __m512 w0 = _mm512_add_ps(_mm512_add_ps(
                _mm512_mul_ps(_mm512_set1_ps(e0_a), px),
                _mm512_mul_ps(_mm512_set1_ps(e0_b), py_vec)),
                _mm512_set1_ps(e0_c));
            
            // Edge 1: w1
            __m512 w1 = _mm512_add_ps(_mm512_add_ps(
                _mm512_mul_ps(_mm512_set1_ps(e1_a), px),
                _mm512_mul_ps(_mm512_set1_ps(e1_b), py_vec)),
                _mm512_set1_ps(e1_c));
            
            // Edge 2: w2
            __m512 w2 = _mm512_add_ps(_mm512_add_ps(
                _mm512_mul_ps(_mm512_set1_ps(e2_a), px),
                _mm512_mul_ps(_mm512_set1_ps(e2_b), py_vec)),
                _mm512_set1_ps(e2_c));
            
            // Mask: inside triangle if all >= 0
            __mmask16 mask = _mm512_cmp_ps_mask(w0, zero, _MM_CMPINT_GE) &
                            _mm512_cmp_ps_mask(w1, zero, _MM_CMPINT_GE) &
                            _mm512_cmp_ps_mask(w2, zero, _MM_CMPINT_GE);
            
            if (mask == 0) continue; // All outside
            
            // Normalize barycentric coordinates
            w0 = _mm512_mul_ps(w0, inv_area_vec);
            w1 = _mm512_mul_ps(w1, inv_area_vec);
            w2 = _mm512_mul_ps(w2, inv_area_vec);
            
            // Interpolate Z
            __m512 z = _mm512_add_ps(_mm512_add_ps(
                _mm512_mul_ps(w0, _mm512_set1_ps(v0->pos.z)),
                _mm512_mul_ps(w1, _mm512_set1_ps(v1->pos.z))),
                _mm512_mul_ps(w2, _mm512_set1_ps(v2->pos.z)));
            
            // Z-test and color write
            // (Simplified - full implementation would do per-pixel Z-test)
            uint32_t base_idx = y * fb->width + x;
            
            // For each active pixel in mask
            for (int i = 0; i < 16; i++) {
                if (mask & (1 << i)) {
                    uint32_t idx = base_idx + i;
                    if (idx >= fb->width * fb->height) continue;
                    
                    float z_val = ((float*)&z)[15 - i]; // Reverse due to set_ps order
                    
                    if (render_state.flags & GFX_RENDER_ZBUFFER) {
                        if (z_val < fb->depth[idx]) {
                            fb->depth[idx] = z_val;
                            // Extract w0, w1, w2 for this pixel
                            float w0v = ((float*)&w0)[15 - i];
                            float w1v = ((float*)&w1)[15 - i];
                            float w2v = ((float*)&w2)[15 - i];
                            fb->color[idx] = shade_pixel(v0, v1, v2, w0v, w1v, w2v, &normal, render_state.flags);
                            render_state.pixels_drawn++;
                        }
                    } else {
                        float w0v = ((float*)&w0)[15 - i];
                        float w1v = ((float*)&w1)[15 - i];
                        float w2v = ((float*)&w2)[15 - i];
                        fb->color[idx] = shade_pixel(v0, v1, v2, w0v, w1v, w2v, &normal, render_state.flags);
                        render_state.pixels_drawn++;
                    }
                }
            }
        }
    }
    
    render_state.triangles_drawn++;
}

// ============================================================================
// AVX2 RASTERIZER (8 pixels)
// ============================================================================

void gfx3d_rasterize_triangle_avx2(vertex_t *v0, vertex_t *v1, vertex_t *v2) {
    if (!simd_has_feature(SIMD_FEATURE_AVX2)) {
        gfx3d_rasterize_triangle(v0, v1, v2);
        return;
    }
    
    // Similar to AVX-512 but with 256-bit vectors (8 pixels)
    // Implementation omitted for brevity - uses __m256
    gfx3d_rasterize_triangle(v0, v1, v2); // Fallback for now
}

// ============================================================================
// SSE RASTERIZER (4 pixels)
// ============================================================================

void gfx3d_rasterize_triangle_sse(vertex_t *v0, vertex_t *v1, vertex_t *v2) {
    if (!simd_has_feature(SIMD_FEATURE_SSE42)) {
        gfx3d_rasterize_triangle(v0, v1, v2);
        return;
    }
    
    // Similar with __m128
    gfx3d_rasterize_triangle(v0, v1, v2); // Fallback
}