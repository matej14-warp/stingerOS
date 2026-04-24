/* scorpion OS - shell/gl.c
   ScorpionGL — software WebGL-like 3D renderer
   CPU rasteriser → VBE framebuffer                                   */

#include "gl.h"
#include "../kernel/vbe.h"
#include "../kernel/kmalloc.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

/* ---- Math helpers ---- */
static float sgl_fabsf(float x) { return x < 0.0f ? -x : x; }
static float sgl_floorf(float x) { return (float)(int)(x >= 0.0f ? (int)x : (int)x - 1); }
static int   sgl_mini(int a, int b) { return a < b ? a : b; }
static int   sgl_maxi(int a, int b) { return a > b ? a : b; }
static float sgl_minf(float a, float b) { return a < b ? a : b; }
static float sgl_maxf(float a, float b) { return a > b ? a : b; }
static float sgl_clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

/* 4x4 matrix multiply: out = a * b (column-major) */
static void mat4_mul(float *out, const float *a, const float *b) {
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)
                s += a[k*4 + row] * b[col*4 + k];
            out[col*4 + row] = s;
        }
    }
}

/* Transform vec4 by 4x4 column-major matrix */
static void mat4_mul_vec4(const float *m, const float *v, float *out) {
    for (int i = 0; i < 4; i++) {
        out[i] = m[0*4+i]*v[0] + m[1*4+i]*v[1] + m[2*4+i]*v[2] + m[3*4+i]*v[3];
    }
}

/* ---- Context ---- */
sgl_ctx_t *g_sgl = (void*)0;

int sgl_init(int width, int height) {
    if (g_sgl) sgl_destroy();
    g_sgl = (sgl_ctx_t*)kzalloc(sizeof(sgl_ctx_t));
    if (!g_sgl) return -1;

    g_sgl->width  = width;
    g_sgl->height = height;

    g_sgl->color_buf = (uint32_t*)kmalloc((size_t)width * height * 4);
    g_sgl->depth_buf = (float*)   kmalloc((size_t)width * height * sizeof(float));
    if (!g_sgl->color_buf || !g_sgl->depth_buf) { sgl_destroy(); return -1; }

    g_sgl->clear_color = 0xFF7FB3FF; /* sky blue default */
    g_sgl->clear_depth = 0.0f;       /* 1/w: furthest = 0 */
    g_sgl->depth_test  = 1;
    g_sgl->cull_face   = 1;

    /* Identity matrices */
    for (int i = 0; i < 16; i++) g_sgl->proj[i] = g_sgl->view[i] = (i%5==0) ? 1.0f : 0.0f;

    /* Allocate one program */
    g_sgl->programs[0].used = 1;

    /* Set up default uniform name table so glGetUniformLocation works */
    sgl_program_t *p = &g_sgl->programs[0];
    p->n_uniforms = 3;
    /* 0 = uProjection */
    p->uniforms[SGL_UNIFORM_PROJECTION].type = SGL_UNI_MAT4;
    for (int i=0;i<32;i++) p->uniforms[SGL_UNIFORM_PROJECTION].name[i]=0;
    { const char *n="uProjection"; for(int i=0;n[i];i++) p->uniforms[SGL_UNIFORM_PROJECTION].name[i]=n[i]; }
    /* 1 = uView */
    p->uniforms[SGL_UNIFORM_VIEW].type = SGL_UNI_MAT4;
    { const char *n="uView"; for(int i=0;n[i];i++) p->uniforms[SGL_UNIFORM_VIEW].name[i]=n[i]; }
    /* 2 = uTexture */
    p->uniforms[SGL_UNIFORM_TEXTURE].type = SGL_UNI_INT;
    { const char *n="uTexture"; for(int i=0;n[i];i++) p->uniforms[SGL_UNIFORM_TEXTURE].name[i]=n[i]; }

    return 0;
}

void sgl_destroy(void) {
    if (!g_sgl) return;
    if (g_sgl->color_buf) kfree(g_sgl->color_buf);
    if (g_sgl->depth_buf) kfree(g_sgl->depth_buf);
    /* free textures */
    for (int i = 0; i < SGL_MAX_TEXTURES; i++)
        if (g_sgl->textures[i].used && g_sgl->textures[i].pixels)
            kfree(g_sgl->textures[i].pixels);
    /* free buffers */
    for (int i = 0; i < SGL_MAX_BUFFERS; i++)
        if (g_sgl->buffers[i].used && g_sgl->buffers[i].data)
            kfree(g_sgl->buffers[i].data);
    kfree(g_sgl);
    g_sgl = (void*)0;
}

void sgl_present(void) {
    if (!g_sgl) return;
    /* Blit colour buffer to VBE back-buffer, then flip to hardware */
    vbe_bb_blit_rect(0, 0, g_sgl->width, g_sgl->height, g_sgl->color_buf);
    vbe_flip();
}

/* ---- State ---- */
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    (void)x; (void)y;
    if (!g_sgl) return;
    /* Re-allocate if size changed */
    if (w == g_sgl->width && h == g_sgl->height) return;
    if (g_sgl->color_buf) kfree(g_sgl->color_buf);
    if (g_sgl->depth_buf) kfree(g_sgl->depth_buf);
    g_sgl->width  = w;
    g_sgl->height = h;
    g_sgl->color_buf = (uint32_t*)kmalloc((size_t)w * h * 4);
    g_sgl->depth_buf = (float*)   kmalloc((size_t)w * h * sizeof(float));
}

void glEnable(GLenum cap) {
    if (!g_sgl) return;
    if (cap == GL_DEPTH_TEST) g_sgl->depth_test = 1;
    if (cap == GL_CULL_FACE)  g_sgl->cull_face  = 1;
}

void glDisable(GLenum cap) {
    if (!g_sgl) return;
    if (cap == GL_DEPTH_TEST) g_sgl->depth_test = 0;
    if (cap == GL_CULL_FACE)  g_sgl->cull_face  = 0;
}

void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (!g_sgl) return;
    (void)a;
    int ri = (int)(sgl_clampf(r,0,1)*255);
    int gi = (int)(sgl_clampf(g,0,1)*255);
    int bi = (int)(sgl_clampf(b,0,1)*255);
    g_sgl->clear_color = (uint32_t)(0xFF000000u | ((uint32_t)ri<<16) | ((uint32_t)gi<<8) | (uint32_t)bi);
}

void glClear(GLbitfield mask) {
    if (!g_sgl) return;
    if (mask & GL_COLOR_BUFFER_BIT) {
        int n = g_sgl->width * g_sgl->height;
        uint32_t *p = g_sgl->color_buf;
        uint32_t c = g_sgl->clear_color;
        for (int i = 0; i < n; i++) p[i] = c;
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
        int n = g_sgl->width * g_sgl->height;
        float *d = g_sgl->depth_buf;
        for (int i = 0; i < n; i++) d[i] = 0.0f;
    }
}

/* ---- Shaders (stubs — we use hard-coded pipeline) ---- */
GLuint glCreateShader(GLenum type) { (void)type; return 1; }
void   glShaderSource(GLuint s, GLsizei c, const char **str, const GLint *len) { (void)s;(void)c;(void)str;(void)len; }
void   glCompileShader(GLuint s) { (void)s; }
void   glGetShaderiv(GLuint s, GLenum p, GLint *out) { (void)s;(void)p; if(out)*out=GL_TRUE; }
void   glDeleteShader(GLuint s) { (void)s; }
GLuint glCreateProgram(void) { return 0; }
void   glAttachShader(GLuint p, GLuint s) { (void)p;(void)s; }
void   glLinkProgram(GLuint p) { (void)p; }
void   glGetProgramiv(GLuint p, GLenum pname, GLint *out) { (void)p;(void)pname; if(out)*out=GL_TRUE; }
void   glUseProgram(GLuint p) { if(g_sgl) g_sgl->cur_program = p; }
void   glGetShaderInfoLog(GLuint s, GLsizei ml, GLsizei *l, char *b) { (void)s;(void)ml; if(l)*l=0; if(b&&ml>0)b[0]=0; }
void   glGetProgramInfoLog(GLuint p, GLsizei ml, GLsizei *l, char *b) { (void)p;(void)ml; if(l)*l=0; if(b&&ml>0)b[0]=0; }

GLint glGetAttribLocation(GLuint program, const char *name) {
    (void)program;
    /* Match the attribute names used in the voxel game shader */
    if (!name) return -1;
    /* "aPosition" → 0, "aTexCoord" → 1, "aShade" → 2 */
    const char *names[3] = { "aPosition", "aTexCoord", "aShade" };
    for (int i = 0; i < 3; i++) {
        const char *n = names[i];
        int j = 0;
        while (n[j] && name[j] && n[j]==name[j]) j++;
        if (!n[j] && !name[j]) return i;
    }
    return -1;
}

static int sgl_scmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

GLint glGetUniformLocation(GLuint program, const char *name) {
    if (!g_sgl || !name) return -1;
    sgl_program_t *p = &g_sgl->programs[0];
    (void)program;
    for (int i = 0; i < p->n_uniforms; i++)
        if (sgl_scmp(p->uniforms[i].name, name) == 0) return i;
    return -1;
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if (!g_sgl || location < 0 || !value) return;
    (void)count; (void)transpose;
    sgl_program_t *p = &g_sgl->programs[0];
    if (location >= p->n_uniforms) return;
    for (int i = 0; i < 16; i++) p->uniforms[location].mat[i] = value[i];
    /* Cache projection/view matrices directly */
    if (location == SGL_UNIFORM_PROJECTION)
        for (int i = 0; i < 16; i++) g_sgl->proj[i] = value[i];
    if (location == SGL_UNIFORM_VIEW)
        for (int i = 0; i < 16; i++) g_sgl->view[i] = value[i];
}

void glUniform1i(GLint location, GLint v0) {
    if (!g_sgl || location < 0) return;
    sgl_program_t *p = &g_sgl->programs[0];
    if (location < p->n_uniforms) p->uniforms[location].ival = v0;
}

/* ---- Buffers ---- */
void glGenBuffers(GLsizei n, GLuint *out) {
    if (!g_sgl || !out) return;
    for (int i = 0; i < n; i++) {
        out[i] = 0;
        for (int j = 1; j < SGL_MAX_BUFFERS; j++) {
            if (!g_sgl->buffers[j].used) {
                g_sgl->buffers[j].used = 1;
                g_sgl->buffers[j].data = (void*)0;
                g_sgl->buffers[j].size = 0;
                out[i] = (GLuint)j;
                break;
            }
        }
    }
}

void glBindBuffer(GLenum target, GLuint buffer) {
    if (!g_sgl) return;
    if (target == GL_ARRAY_BUFFER) g_sgl->bound_array_buffer = buffer;
}

void glBufferData(GLenum target, size_t size, const void *data, GLenum usage) {
    if (!g_sgl) return;
    (void)usage;
    GLuint id = (target == GL_ARRAY_BUFFER) ? g_sgl->bound_array_buffer : 0;
    if (!id || id >= SGL_MAX_BUFFERS) return;
    sgl_buffer_t *b = &g_sgl->buffers[id];
    if (b->data) { kfree(b->data); b->data = (void*)0; }
    b->data = (float*)kmalloc(size);
    if (!b->data) { b->size = 0; return; }
    b->size = size;
    if (data) {
        const uint8_t *src = (const uint8_t*)data;
        uint8_t       *dst = (uint8_t*)b->data;
        for (size_t i = 0; i < size; i++) dst[i] = src[i];
    }
}

void glDeleteBuffers(GLsizei n, const GLuint *bufs) {
    if (!g_sgl || !bufs) return;
    for (int i = 0; i < n; i++) {
        GLuint id = bufs[i];
        if (id < SGL_MAX_BUFFERS && g_sgl->buffers[id].used) {
            if (g_sgl->buffers[id].data) kfree(g_sgl->buffers[id].data);
            g_sgl->buffers[id].used = 0;
            g_sgl->buffers[id].data = (void*)0;
            g_sgl->buffers[id].size = 0;
        }
    }
}

/* ---- Vertex attributes ---- */
void glEnableVertexAttribArray(GLuint index) {
    if (!g_sgl || index >= SGL_MAX_ATTRIBS) return;
    g_sgl->attribs[index].enabled = 1;
}

void glDisableVertexAttribArray(GLuint index) {
    if (!g_sgl || index >= SGL_MAX_ATTRIBS) return;
    g_sgl->attribs[index].enabled = 0;
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                            GLsizei stride, size_t offset) {
    if (!g_sgl || index >= SGL_MAX_ATTRIBS) return;
    (void)normalized;
    sgl_attrib_t *a = &g_sgl->attribs[index];
    a->buffer = g_sgl->bound_array_buffer;
    a->size   = size;
    a->type   = type;
    a->stride = stride;
    a->offset = offset;
}

/* ---- Textures ---- */
void glGenTextures(GLsizei n, GLuint *out) {
    if (!g_sgl || !out) return;
    for (int i = 0; i < n; i++) {
        out[i] = 0;
        for (int j = 1; j < SGL_MAX_TEXTURES; j++) {
            if (!g_sgl->textures[j].used) {
                g_sgl->textures[j].used       = 1;
                g_sgl->textures[j].pixels     = (void*)0;
                g_sgl->textures[j].wrap_s     = GL_REPEAT;
                g_sgl->textures[j].wrap_t     = GL_REPEAT;
                g_sgl->textures[j].min_filter = GL_NEAREST;
                g_sgl->textures[j].mag_filter = GL_NEAREST;
                out[i] = (GLuint)j;
                break;
            }
        }
    }
}

void glBindTexture(GLenum target, GLuint texture) {
    if (!g_sgl) return;
    if (target == GL_TEXTURE_2D) g_sgl->bound_texture = texture;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                   GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const void *pixels) {
    if (!g_sgl) return;
    (void)target; (void)level; (void)internalformat; (void)border; (void)type;
    GLuint id = g_sgl->bound_texture;
    if (!id || id >= SGL_MAX_TEXTURES) return;
    sgl_texture_t *t = &g_sgl->textures[id];
    if (t->pixels) { kfree(t->pixels); t->pixels = (void*)0; }
    t->width  = width;
    t->height = height;
    t->pixels = (uint32_t*)kmalloc((size_t)width * height * 4);
    if (!t->pixels) return;

    if (!pixels) return;
    /* Convert input pixels to ARGB uint32_t */
    const uint8_t *src = (const uint8_t*)pixels;
    int n = width * height;
    if (format == GL_RGBA) {
        for (int i = 0; i < n; i++) {
            uint8_t r=src[i*4], g=src[i*4+1], b=src[i*4+2], a=src[i*4+3];
            t->pixels[i] = ((uint32_t)a<<24)|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
        }
    } else if (format == GL_RGB) {
        for (int i = 0; i < n; i++) {
            uint8_t r=src[i*3], g=src[i*3+1], b=src[i*3+2];
            t->pixels[i] = 0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
        }
    } else {
        /* Treat as RGBA */
        for (int i = 0; i < n; i++) {
            uint8_t r=src[i*4], g2=src[i*4+1], b=src[i*4+2], a=src[i*4+3];
            t->pixels[i] = ((uint32_t)a<<24)|((uint32_t)r<<16)|((uint32_t)g2<<8)|b;
        }
    }
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    if (!g_sgl) return;
    (void)target;
    GLuint id = g_sgl->bound_texture;
    if (!id || id >= SGL_MAX_TEXTURES) return;
    sgl_texture_t *t = &g_sgl->textures[id];
    if (pname == GL_TEXTURE_WRAP_S)     t->wrap_s     = (GLenum)param;
    if (pname == GL_TEXTURE_WRAP_T)     t->wrap_t     = (GLenum)param;
    if (pname == GL_TEXTURE_MIN_FILTER) t->min_filter = (GLenum)param;
    if (pname == GL_TEXTURE_MAG_FILTER) t->mag_filter = (GLenum)param;
}

void glDeleteTextures(GLsizei n, const GLuint *texs) {
    if (!g_sgl || !texs) return;
    for (int i = 0; i < n; i++) {
        GLuint id = texs[i];
        if (id < SGL_MAX_TEXTURES && g_sgl->textures[id].used) {
            if (g_sgl->textures[id].pixels) kfree(g_sgl->textures[id].pixels);
            g_sgl->textures[id].used   = 0;
            g_sgl->textures[id].pixels = (void*)0;
        }
    }
}

/* ================================================================
   RASTERISER
   ================================================================ */

/* Sample a texture at (u,v) — nearest neighbour, returns ARGB */
static uint32_t sgl_sample_texture(sgl_texture_t *t, float u, float v) {
    if (!t || !t->pixels) return 0xFF808080u;

    /* Wrap */
    u = u - sgl_floorf(u);
    v = v - sgl_floorf(v);

    int tx = (int)(u * (float)t->width);
    int ty = (int)(v * (float)t->height);
    if (tx < 0) tx = 0;
    if (ty < 0) ty = 0;
    if (tx >= t->width)  tx = t->width  - 1;
    if (ty >= t->height) ty = t->height - 1;

    return t->pixels[ty * t->width + tx];
}

/* Fetch float attribute for vertex i from attrib slot */
static float sgl_fetch_attrib_f(sgl_attrib_t *a, int vertex_idx, int component) {
    if (!a->enabled || !a->buffer || a->buffer >= SGL_MAX_BUFFERS) return 0.0f;
    sgl_buffer_t *b = &g_sgl->buffers[a->buffer];
    if (!b->data) return 0.0f;
    int stride_floats = a->stride ? a->stride / 4 : a->size;
    size_t byte_off = a->offset + (size_t)vertex_idx * (size_t)a->stride + (size_t)component * 4;
    if (byte_off + 4 > b->size) return 0.0f;
    float val;
    uint8_t *src = (uint8_t*)b->data + a->offset + (size_t)vertex_idx * (size_t)(a->stride ? a->stride : a->size*4) + (size_t)component * 4;
    /* Safe float read */
    uint8_t tmp[4]; tmp[0]=src[0]; tmp[1]=src[1]; tmp[2]=src[2]; tmp[3]=src[3];
    val = *((float*)tmp);
    (void)stride_floats; (void)byte_off;
    return val;
}

/* Draw a single triangle given 3 pre-transformed screen vertices.
   Each vertex: { sx, sy, invw, u*invw, v*invw, shade*invw }        */
typedef struct { float sx, sy, invw, u_over_w, v_over_w, shade_over_w; } sgl_sv_t;

static void sgl_draw_triangle(sgl_sv_t v0, sgl_sv_t v1, sgl_sv_t v2, sgl_texture_t *tex) {
    int W = g_sgl->width;
    int H = g_sgl->height;

    /* Backface cull (counter-clockwise winding = front in WebGL) */
    if (g_sgl->cull_face) {
        float cross = (v1.sx - v0.sx) * (v2.sy - v0.sy)
                    - (v1.sy - v0.sy) * (v2.sx - v0.sx);
        if (cross >= 0.0f) return;   /* back face */
    }

    /* Bounding box */
    int minx = (int)sgl_minf(v0.sx, sgl_minf(v1.sx, v2.sx));
    int miny = (int)sgl_minf(v0.sy, sgl_minf(v1.sy, v2.sy));
    int maxx = (int)(sgl_maxf(v0.sx, sgl_maxf(v1.sx, v2.sx)) + 1.0f);
    int maxy = (int)(sgl_maxf(v0.sy, sgl_maxf(v1.sy, v2.sy)) + 1.0f);

    minx = sgl_maxi(minx, 0);
    miny = sgl_maxi(miny, 0);
    maxx = sgl_mini(maxx, W - 1);
    maxy = sgl_mini(maxy, H - 1);

    if (minx > maxx || miny > maxy) return;

    /* Edge function setup */
    float dx01 = v1.sx - v0.sx, dy01 = v1.sy - v0.sy;
    float dx12 = v2.sx - v1.sx, dy12 = v2.sy - v1.sy;
    float dx20 = v0.sx - v2.sx, dy20 = v0.sy - v2.sy;

    float area2 = dx01 * (v2.sy - v0.sy) - dy01 * (v2.sx - v0.sx);
    if (sgl_fabsf(area2) < 0.001f) return;
    float inv_area2 = 1.0f / area2;

    /* Starting edge values at (minx+0.5, miny+0.5) */
    float px = (float)minx + 0.5f, py = (float)miny + 0.5f;
    float e0_row = (px - v0.sx)*dy01 - (py - v0.sy)*dx01;
    float e1_row = (px - v1.sx)*dy12 - (py - v1.sy)*dx12;
    float e2_row = (px - v2.sx)*dy20 - (py - v2.sy)*dx20;

    for (int y = miny; y <= maxy; y++) {
        float e0 = e0_row, e1 = e1_row, e2 = e2_row;
        for (int x = minx; x <= maxx; x++) {
            /* Inside triangle? */
            if (e0 >= 0.0f && e1 >= 0.0f && e2 >= 0.0f) {
                /* Barycentric coords */
                float w0 = e0 * inv_area2;
                float w1 = e1 * inv_area2;
                float w2 = 1.0f - w0 - w1;

                /* Perspective-correct interpolation */
                float invw = v0.invw*w2 + v1.invw*w0 + v2.invw*w1;

                /* Depth test (larger invw = closer) */
                int idx = y * W + x;
                if (g_sgl->depth_test) {
                    if (invw <= g_sgl->depth_buf[idx]) goto next_pixel;
                    g_sgl->depth_buf[idx] = invw;
                }

                {
                    float u_w = v0.u_over_w*w2 + v1.u_over_w*w0 + v2.u_over_w*w1;
                    float v_w = v0.v_over_w*w2 + v1.v_over_w*w0 + v2.v_over_w*w1;
                    float sh_w = v0.shade_over_w*w2 + v1.shade_over_w*w0 + v2.shade_over_w*w1;

                    float w_recip = (invw > 0.0f) ? (1.0f / invw) : 0.0f;
                    float u     = u_w  * w_recip;
                    float v_tex = v_w  * w_recip;
                    float shade = sh_w * w_recip;

                    /* Sample texture */
                    uint32_t texel = sgl_sample_texture(tex, u, v_tex);

                    /* Alpha test */
                    uint8_t a = (uint8_t)(texel >> 24);
                    if (a < 128) goto next_pixel;

                    /* Apply shade */
                    uint8_t r = (uint8_t)((float)((texel>>16)&0xFF) * shade);
                    uint8_t g = (uint8_t)((float)((texel>> 8)&0xFF) * shade);
                    uint8_t b = (uint8_t)((float)( texel     &0xFF) * shade);

                    g_sgl->color_buf[idx] = 0xFF000000u |
                        ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
                }
            }
            next_pixel:
            e0 += dy01;
            e1 += dy12;
            e2 += dy20;
        }
        e0_row -= dx01;
        e1_row -= dx12;
        e2_row -= dx20;
    }
}

/* ---- glDrawArrays ---- */
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!g_sgl || mode != GL_TRIANGLES || count < 3) return;

    int W = g_sgl->width;
    int H = g_sgl->height;
    float half_w = (float)W * 0.5f;
    float half_h = (float)H * 0.5f;

    /* Combined MVP = proj * view */
    float mvp[16];
    mat4_mul(mvp, g_sgl->proj, g_sgl->view);

    /* Active texture */
    sgl_texture_t *tex = (void*)0;
    if (g_sgl->bound_texture && g_sgl->bound_texture < SGL_MAX_TEXTURES)
        tex = &g_sgl->textures[g_sgl->bound_texture];

    /* Get attrib pointers */
    sgl_attrib_t *a_pos   = &g_sgl->attribs[SGL_ATTRIB_POSITION];
    sgl_attrib_t *a_tex   = &g_sgl->attribs[SGL_ATTRIB_TEXCOORD];
    sgl_attrib_t *a_shade = &g_sgl->attribs[SGL_ATTRIB_SHADE];

    int n_tris = count / 3;
    for (int ti = 0; ti < n_tris; ti++) {
        sgl_sv_t sv[3];
        int clipped = 0;

        for (int vi = 0; vi < 3; vi++) {
            int vi_idx = first + ti*3 + vi;

            /* Fetch position */
            float px = sgl_fetch_attrib_f(a_pos, vi_idx, 0);
            float py = sgl_fetch_attrib_f(a_pos, vi_idx, 1);
            float pz = sgl_fetch_attrib_f(a_pos, vi_idx, 2);
            float pw = 1.0f;

            /* Fetch texcoord */
            float tu = a_tex->enabled ? sgl_fetch_attrib_f(a_tex, vi_idx, 0) : 0.0f;
            float tv = a_tex->enabled ? sgl_fetch_attrib_f(a_tex, vi_idx, 1) : 0.0f;

            /* Fetch shade */
            float shade = a_shade->enabled ? sgl_fetch_attrib_f(a_shade, vi_idx, 0) : 1.0f;

            /* Transform by MVP */
            float in4[4]  = { px, py, pz, pw };
            float out4[4];
            mat4_mul_vec4(mvp, in4, out4);

            /* Clip against near plane (w > 0) */
            if (out4[3] <= 0.001f) { clipped = 1; break; }

            float inv_w = 1.0f / out4[3];

            /* NDC → screen */
            float sx = ( out4[0] * inv_w + 1.0f) * half_w;
            float sy = (-out4[1] * inv_w + 1.0f) * half_h;

            sv[vi].sx          = sx;
            sv[vi].sy          = sy;
            sv[vi].invw        = inv_w;
            sv[vi].u_over_w    = tu    * inv_w;
            sv[vi].v_over_w    = tv    * inv_w;
            sv[vi].shade_over_w= shade * inv_w;
        }

        if (clipped) continue;
        sgl_draw_triangle(sv[0], sv[1], sv[2], tex);
    }
}
