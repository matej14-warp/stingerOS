#include "femboygl.h"
#include <stdint.h>
#include <stddef.h>

/* ---- Math helpers ---- */
static float _fabs(float a)       { return a < 0 ? -a : a; }
static float _fmax(float a, float b) { return a > b ? a : b; }
static float _fmin(float a, float b) { return a < b ? a : b; }

/* Integer sqrt via Newton–Raphson (for normalisation) */
static float _fsqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float r = x;
    for (int i = 0; i < 16; i++) r = 0.5f * (r + x / r);
    return r;
}

/* Sine approximation — input in radians, full-precision poly */
static float _fsin(float x) {
    /* reduce to [-π, π] */
    const float PI  = 3.14159265f;
    const float TAU = 6.28318530f;
    while (x >  PI) x -= TAU;
    while (x < -PI) x += TAU;
    /* 5-term Taylor: x - x³/6 + x⁵/120 - x⁷/5040 + x⁹/362880 */
    float x2 = x * x;
    return x * (1.0f + x2 * (-0.16666667f + x2 * (0.00833333f + x2 * (-0.00019841f + x2 * 0.00000276f))));
}
static float _fcos(float x) { return _fsin(x + 1.57079633f); }

/* ================================================================ gl_init */
void gl_init(gl_context_t *ctx, int w, int h, uint32_t *color_buf, float *depth_buf) {
    ctx->width        = w;
    ctx->height       = h;
    ctx->color_buffer = color_buf;
    ctx->depth_buffer = depth_buf;
    ctx->texture      = NULL;
    ctx->current_color = 0xFFFFFFFF;
    gl_load_identity(ctx);
}

/* ================================================================ gl_clear */
void gl_clear(gl_context_t *ctx, uint32_t color) {
    int size = ctx->width * ctx->height;
    for (int i = 0; i < size; i++) {
        ctx->color_buffer[i] = color;
        ctx->depth_buffer[i] = 1.0f; /* NDC depth: 1.0 = far */
    }
}

/* ================================================================ gl_load_identity */
void gl_load_identity(gl_context_t *ctx) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            ctx->model.m[i][j]      = (i == j) ? 1.0f : 0.0f;
            ctx->view.m[i][j]       = (i == j) ? 1.0f : 0.0f;
            ctx->projection.m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
}

/* ================================================================ Matrix helpers */

/* Multiply two 4x4 matrices: out = a * b */
static void mat4_mul(mat4_t *out, const mat4_t *a, const mat4_t *b) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a->m[r][k] * b->m[k][c];
            out->m[r][c] = s;
        }
}

/* Multiply matrix by column vector (w=1 for points) */
static vec4_t mat4_mul_vec4(const mat4_t *m, vec4_t v) {
    vec4_t r;
    r.x = m->m[0][0]*v.x + m->m[0][1]*v.y + m->m[0][2]*v.z + m->m[0][3]*v.w;
    r.y = m->m[1][0]*v.x + m->m[1][1]*v.y + m->m[1][2]*v.z + m->m[1][3]*v.w;
    r.z = m->m[2][0]*v.x + m->m[2][1]*v.y + m->m[2][2]*v.z + m->m[2][3]*v.w;
    r.w = m->m[3][0]*v.x + m->m[3][1]*v.y + m->m[3][2]*v.z + m->m[3][3]*v.w;
    return r;
}

/* ================================================================ gl_perspective
   Standard OpenGL-style perspective matrix (column-major stored row-major here).
   fov = vertical FOV in radians, aspect = w/h, near/far = clip planes.
*/
void gl_perspective(gl_context_t *ctx, float fov, float aspect, float near, float far) {
    float f = 1.0f / _fsin(fov * 0.5f);
    if (_fabs(_fcos(fov * 0.5f)) > 0.0001f) {
        /* f = cos(fov/2) / sin(fov/2) */
        f = _fcos(fov * 0.5f) / _fsin(fov * 0.5f);
    }
    float range = near - far;
    mat4_t p = {{{0}}};
    p.m[0][0] = f / aspect;
    p.m[1][1] = f;
    p.m[2][2] = (near + far) / range;       /* -(f+n)/(f-n) */
    p.m[2][3] = (2.0f * near * far) / range; /* -2fn/(f-n)  */
    p.m[3][2] = -1.0f;
    /* p.m[3][3] = 0 */
    ctx->projection = p;
}

/* ================================================================ gl_look_at
   Builds a view matrix from eye position, target, and world-up vector.
*/
void gl_look_at(gl_context_t *ctx, vec3_t eye, vec3_t center, vec3_t up) {
    /* forward = normalize(center - eye) */
    float fx = center.x - eye.x;
    float fy = center.y - eye.y;
    float fz = center.z - eye.z;
    float fn = _fsqrt(fx*fx + fy*fy + fz*fz);
    if (fn < 0.00001f) fn = 0.00001f;
    fx /= fn; fy /= fn; fz /= fn;

    /* right = normalize(forward × up) */
    float rx = fy*up.z - fz*up.y;
    float ry = fz*up.x - fx*up.z;
    float rz = fx*up.y - fy*up.x;
    float rn = _fsqrt(rx*rx + ry*ry + rz*rz);
    if (rn < 0.00001f) rn = 0.00001f;
    rx /= rn; ry /= rn; rz /= rn;

    /* newup = right × forward */
    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    mat4_t v = {{{0}}};
    v.m[0][0] =  rx; v.m[0][1] =  ry; v.m[0][2] =  rz; v.m[0][3] = -(rx*eye.x + ry*eye.y + rz*eye.z);
    v.m[1][0] =  ux; v.m[1][1] =  uy; v.m[1][2] =  uz; v.m[1][3] = -(ux*eye.x + uy*eye.y + uz*eye.z);
    v.m[2][0] = -fx; v.m[2][1] = -fy; v.m[2][2] = -fz; v.m[2][3] =  (fx*eye.x + fy*eye.y + fz*eye.z);
    v.m[3][3] = 1.0f;
    ctx->view = v;
}

/* ================================================================ gl_transform_vertex
   Applies model → view → projection, then perspective-divide → NDC → screen.
   Returns 1 if visible (in front of near plane), 0 if clipped.
*/
int gl_transform_vertex(gl_context_t *ctx, vec3_t world, vec3_t *screen_out, float *depth_out) {
    /* Build MVP = projection * view * model */
    mat4_t mv, mvp;
    mat4_mul(&mv,  &ctx->view,       &ctx->model);
    mat4_mul(&mvp, &ctx->projection, &mv);

    vec4_t v = { world.x, world.y, world.z, 1.0f };
    vec4_t c = mat4_mul_vec4(&mvp, v);

    if (c.w <= 0.0001f) return 0; /* behind near plane */

    /* perspective divide → NDC */
    float ndx = c.x / c.w;
    float ndy = c.y / c.w;
    float ndz = c.z / c.w;

    /* cull outside NDC cube */
    if (ndx < -1.0f || ndx > 1.0f || ndy < -1.0f || ndy > 1.0f || ndz < -1.0f || ndz > 1.0f)
        return 0;

    /* viewport transform */
    screen_out->x = (ndx + 1.0f) * 0.5f * (float)(ctx->width  - 1);
    screen_out->y = (1.0f - ndy) * 0.5f * (float)(ctx->height - 1); /* flip Y */
    screen_out->z = ndz;

    if (depth_out) *depth_out = ndz;
    return 1;
}

/* ================================================================ gl_put_pixel (internal) */
static inline void gl_put_pixel(gl_context_t *ctx, int x, int y, float z, float u, float v) {
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) return;
    int idx = y * ctx->width + x;
    if (ctx->depth_buffer[idx] > z) {
        ctx->depth_buffer[idx] = z;
        uint32_t color = ctx->current_color;
        if (ctx->texture) {
            int tx = (int)(u * (float)(ctx->tex_width  - 1)) % ctx->tex_width;
            int ty = (int)(v * (float)(ctx->tex_height - 1)) % ctx->tex_height;
            if (tx < 0) tx += ctx->tex_width;
            if (ty < 0) ty += ctx->tex_height;
            color = ctx->texture[ty * ctx->tex_width + tx];
        }
        if (color >> 24 != 0 || (color & 0xFFFFFF) != 0) /* skip pure-black-with-alpha-0 */
            ctx->color_buffer[idx] = color;
    }
}

/* ================================================================ gl_draw_triangle
   Barycentric rasterizer.  v1/v2/v3 are already in screen space (pixels, z in NDC).
   t1/t2/t3 are texture coords (x=u, y=v).
*/
void gl_draw_triangle(gl_context_t *ctx,
                      vec3_t v1, vec3_t v2, vec3_t v3,
                      vec3_t t1, vec3_t t2, vec3_t t3) {
    int min_x = (int)_fmin(v1.x, _fmin(v2.x, v3.x));
    int max_x = (int)_fmax(v1.x, _fmax(v2.x, v3.x));
    int min_y = (int)_fmin(v1.y, _fmin(v2.y, v3.y));
    int max_y = (int)_fmax(v1.y, _fmax(v2.y, v3.y));

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= ctx->width)  max_x = ctx->width  - 1;
    if (max_y >= ctx->height) max_y = ctx->height - 1;

    float den = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);
    if (den > -0.0001f && den < 0.0001f) return; /* degenerate */

    float inv_den = 1.0f / den;

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float w1 = ((v2.y - v3.y) * (x - v3.x) + (v3.x - v2.x) * (y - v3.y)) * inv_den;
            float w2 = ((v3.y - v1.y) * (x - v3.x) + (v1.x - v3.x) * (y - v3.y)) * inv_den;
            float w3 = 1.0f - w1 - w2;

            if (w1 >= 0.0f && w2 >= 0.0f && w3 >= 0.0f) {
                float z = w1 * v1.z + w2 * v2.z + w3 * v3.z;
                float u = w1 * t1.x + w2 * t2.x + w3 * t3.x;
                float v = w1 * t1.y + w2 * t2.y + w3 * t3.y;
                gl_put_pixel(ctx, x, y, z, u, v);
            }
        }
    }
}

/* ================================================================ gl_draw_triangle_3d
   Convenience: transform 3 world-space vertices then rasterize.
   Returns 0 if fully clipped.
*/
int gl_draw_triangle_3d(gl_context_t *ctx,
                        vec3_t w1, vec3_t w2, vec3_t w3,
                        vec3_t t1, vec3_t t2, vec3_t t3) {
    vec3_t s1, s2, s3;
    if (!gl_transform_vertex(ctx, w1, &s1, NULL)) return 0;
    if (!gl_transform_vertex(ctx, w2, &s2, NULL)) return 0;
    if (!gl_transform_vertex(ctx, w3, &s3, NULL)) return 0;
    gl_draw_triangle(ctx, s1, s2, s3, t1, t2, t3);
    return 1;
}

/* ================================================================ gl_set_texture */
void gl_set_texture(gl_context_t *ctx, uint32_t *tex, int w, int h) {
    ctx->texture    = tex;
    ctx->tex_width  = w;
    ctx->tex_height = h;
}

/* ================================================================ gl_matrix_mode (stub — kept for compat) */
void gl_matrix_mode(int mode) { (void)mode; }
