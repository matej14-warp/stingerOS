#ifndef FEMBOYGL_H
#define FEMBOYGL_H

#include <stdint.h>
#include <stddef.h>

/* ================================================================
   FemboyGL v2.0 — Software 3D rasterizer for ScorpionOS
   ================================================================
   Features:
   - Full MVP matrix pipeline (model / view / projection)
   - Perspective-correct barycentric rasterization
   - Per-pixel depth test
   - Optional texture mapping
   - gl_perspective() — standard perspective projection
   - gl_look_at()     — camera view matrix
   - gl_draw_triangle_3d() — transform + rasterize in one call
*/

typedef struct { float x, y, z;    } vec3_t;
typedef struct { float x, y, z, w; } vec4_t;
typedef struct { float m[4][4];    } mat4_t;

typedef struct {
    /* framebuffer */
    uint32_t *color_buffer;
    float    *depth_buffer;
    int       width, height;

    /* matrices */
    mat4_t model, view, projection;

    /* current draw state */
    uint32_t  current_color;
    uint32_t *texture;
    int       tex_width, tex_height;
} gl_context_t;

/* ----- Core ----- */
void gl_init       (gl_context_t *ctx, int w, int h, uint32_t *color_buf, float *depth_buf);
void gl_clear      (gl_context_t *ctx, uint32_t color);
void gl_load_identity(gl_context_t *ctx);
void gl_matrix_mode(int mode); /* stub — kept for compatibility */

/* ----- Matrices ----- */

/* Perspective projection. fov = vertical FOV in radians, aspect = w/h */
void gl_perspective(gl_context_t *ctx, float fov, float aspect, float near, float far);

/* Camera: eye = camera position, center = look-at point, up = world-up */
void gl_look_at    (gl_context_t *ctx, vec3_t eye, vec3_t center, vec3_t up);

/* ----- Rendering ----- */

/* Low-level: rasterize triangle with screen-space vertices (already projected) */
void gl_draw_triangle(gl_context_t *ctx,
                      vec3_t v1, vec3_t v2, vec3_t v3,
                      vec3_t t1, vec3_t t2, vec3_t t3);

/* High-level: transform world-space vertices through MVP then rasterize.
   Returns 1 if drawn, 0 if clipped. */
int  gl_draw_triangle_3d(gl_context_t *ctx,
                         vec3_t w1, vec3_t w2, vec3_t w3,
                         vec3_t t1, vec3_t t2, vec3_t t3);

/* Transform a single world-space vertex to screen space.
   Returns 1 if visible, 0 if behind near plane or outside NDC cube. */
int  gl_transform_vertex(gl_context_t *ctx, vec3_t world,
                         vec3_t *screen_out, float *depth_out);

/* ----- Texturing ----- */
void gl_set_texture(gl_context_t *ctx, uint32_t *tex, int w, int h);

#endif /* FEMBOYGL_H */
