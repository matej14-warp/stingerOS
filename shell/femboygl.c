









float _fabs (float a)            { return a < 0.0f ? -a : a; }
float _fmin (float a, float b)   { return a < b ? a : b; }
float _fmax (float a, float b)   { return a > b ? a : b; }
float _fsqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float r = x;
    for (int i = 0; i < 16; i++) r = 0.5f * (r + x / r);
    return r;
}
float _fsin(float x) {
    const float INV_TAU = 0.15915494309f;
    const float TAU = 6.28318530717959f;
    x -= TAU * (float)(int)(x * INV_TAU + (x >= 0.0f ? 0.5f : -0.5f));
    float x2 = x * x;
    return x * (1.0f + x2 * (-0.16666667f + x2 * (0.00833333f + x2 * (-0.00019841f + x2 * 0.00000276f))));
}
float _fcos(float x) { return _fsin(x + 1.57079632679f); }
float _ftan(float x) {
    float c = _fcos(x);
    return (c != 0.0f) ? _fsin(x) / c : 1e38f;
}


static void mat4_mul(mat4_t *out, const mat4_t *a, const mat4_t *b) {
    mat4_t tmp;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a->m[r][k] * b->m[k][c];
            tmp.m[r][c] = s;
        }
    *out = tmp;
}

static vec4_t mat4_mul_vec4(const mat4_t *m, vec4_t v) {
    vec4_t r;
    r.x = m->m[0][0]*v.x + m->m[0][1]*v.y + m->m[0][2]*v.z + m->m[0][3]*v.w;
    r.y = m->m[1][0]*v.x + m->m[1][1]*v.y + m->m[1][2]*v.z + m->m[1][3]*v.w;
    r.z = m->m[2][0]*v.x + m->m[2][1]*v.y + m->m[2][2]*v.z + m->m[2][3]*v.w;
    r.w = m->m[3][0]*v.x + m->m[3][1]*v.y + m->m[3][2]*v.z + m->m[3][3]*v.w;
    return r;
}


static void fgl_mat4_mul(float *out, const float *a, const float *b) {
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a[k*4 + row] * b[col*4 + k];
            out[col*4 + row] = s;
        }
    }
}


void gl_init(gl_context_t *ctx, int w, int h, uint32_t *color_buf, float *depth_buf) {
    for (uint8_t *p = (uint8_t*)ctx; p < (uint8_t*)ctx + sizeof(gl_context_t); p++) *p = 0;
    ctx->width         = w;
    ctx->height        = h;
    ctx->color_buffer  = color_buf;
    ctx->depth_buffer  = depth_buf;
    ctx->current_color = 0xFFFFFFFF;
    ctx->cull_backface = 1;
    ctx->depth_test_enabled = 1;
    gl_load_identity(ctx);
}

void gl_clear(gl_context_t *ctx, uint32_t color) {
    int n = ctx->width * ctx->height;
    for (int i = 0; i < n; i++) {
        ctx->color_buffer[i] = color;
        ctx->depth_buffer[i] = 1.0f;
    }
}

void gl_load_identity(gl_context_t *ctx) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            ctx->model.m[i][j]      = (i == j) ? 1.0f : 0.0f;
            ctx->view.m[i][j]       = (i == j) ? 1.0f : 0.0f;
            ctx->projection.m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
}

void gl_set_color(gl_context_t *ctx, uint32_t color) { ctx->current_color = color; }
void gl_set_texture(gl_context_t *ctx, uint32_t *tex, int w, int h) {
    ctx->texture = tex; ctx->tex_width = w; ctx->tex_height = h;
}
void gl_set_culling(gl_context_t *ctx, int enable) { ctx->cull_backface = enable; }

void gl_perspective(gl_context_t *ctx, float fov, float aspect, float near, float far) {
    float f = 1.0f / _ftan(fov * 0.5f);
    float range = near - far;
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) ctx->projection.m[i][j] = 0.0f;
    ctx->projection.m[0][0] = f / aspect;
    ctx->projection.m[1][1] = f;
    ctx->projection.m[2][2] = (far + near) / range;
    ctx->projection.m[2][3] = (2.0f * far * near) / range;
    ctx->projection.m[3][2] = -1.0f;
}

void gl_ortho(gl_context_t *ctx, float l, float r, float b, float t, float n, float f) {
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) ctx->projection.m[i][j] = 0.0f;
    ctx->projection.m[0][0] = 2.0f / (r - l);
    ctx->projection.m[0][3] = -(r + l) / (r - l);
    ctx->projection.m[1][1] = 2.0f / (t - b);
    ctx->projection.m[1][3] = -(t + b) / (t - b);
    ctx->projection.m[2][2] = -2.0f / (f - n);
    ctx->projection.m[2][3] = -(f + n) / (f - n);
    ctx->projection.m[3][3] = 1.0f;
}

void gl_look_at(gl_context_t *ctx, vec3_t eye, vec3_t center, vec3_t up) {
    float fx = center.x - eye.x, fy = center.y - eye.y, fz = center.z - eye.z;
    float fn = _fsqrt(fx*fx + fy*fy + fz*fz);
    if (fn < 1e-7f) fn = 1e-7f;
    fx /= fn; fy /= fn; fz /= fn;
    float rx = fy*up.z - fz*up.y, ry = fz*up.x - fx*up.z, rz = fx*up.y - fy*up.x;
    float rn = _fsqrt(rx*rx + ry*ry + rz*rz);
    if (rn < 1e-7f) rn = 1e-7f;
    rx /= rn; ry /= rn; rz /= rn;
    float ux = ry*fz - rz*fy, uy = rz*fx - rx*fz, uz = rx*fy - ry*fx;
    mat4_t v;
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) v.m[i][j] = 0.0f;
    v.m[0][0] =  rx;  v.m[0][1] =  ry;  v.m[0][2] =  rz;
    v.m[0][3] = -(rx*eye.x + ry*eye.y + rz*eye.z);
    v.m[1][0] =  ux;  v.m[1][1] =  uy;  v.m[1][2] =  uz;
    v.m[1][3] = -(ux*eye.x + uy*eye.y + uz*eye.z);
    v.m[2][0] = -fx;  v.m[2][1] = -fy;  v.m[2][2] = -fz;
    v.m[2][3] =  (fx*eye.x + fy*eye.y + fz*eye.z);
    v.m[3][3] = 1.0f;
    ctx->view = v;
}

void gl_model_mul(gl_context_t *ctx, const mat4_t *m) { mat4_mul(&ctx->model, &ctx->model, m); }


static inline int _umod(int v, int m) { v %= m; return v < 0 ? v + m : v; }

static inline void _put_pixel(gl_context_t *ctx, int x, int y, float z, float u, float v, float shade) {
    if ((unsigned)x >= (unsigned)ctx->width || (unsigned)y >= (unsigned)ctx->height) return;
    int idx = y * ctx->width + x;
    if (ctx->depth_test_enabled && z >= ctx->depth_buffer[idx]) return;
    ctx->depth_buffer[idx] = z;
    uint32_t color;
    if (ctx->texture) {
        int tx = _umod((int)(u * (float)ctx->tex_width),  ctx->tex_width);
        int ty = _umod((int)(v * (float)ctx->tex_height), ctx->tex_height);
        color = ctx->texture[ty * ctx->tex_width + tx];
        if ((color >> 24) < 128) { ctx->depth_buffer[idx] = z; return; }
    } else color = ctx->current_color;
    if (shade != 1.0f) {
        uint8_t r = (uint8_t)((float)((color>>16)&0xFF) * shade);
        uint8_t g = (uint8_t)((float)((color>> 8)&0xFF) * shade);
        uint8_t b = (uint8_t)((float)( color    &0xFF) * shade);
        color = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
    ctx->color_buffer[idx] = color;
}

typedef struct {
    gl_context_t *ctx;
    vec3_t v1, v2, v3, t1, t2, t3;
    float iw1, iw2, iw3;
    float inv_den;
    float b1_x_step, b1_y_step, b2_x_step, b2_y_step;
    float b1_row_start0, b2_row_start0;
    int x0, x1, y0;
} triangle_task_t;

static void parallel_triangle_task(void *arg, int start, int end) {
    triangle_task_t *p = arg;
    gl_context_t *ctx = p->ctx;

    float b1_row_start = p->b1_row_start0 + start * p->b1_y_step;
    float b2_row_start = p->b2_row_start0 + start * p->b2_y_step;

    for (int i = start; i < end; i++) {
        int y = p->y0 + i;
        float b1 = b1_row_start;
        float b2 = b2_row_start;
        uint32_t *line_color = &ctx->color_buffer[y * ctx->width];
        float *line_depth = &ctx->depth_buffer[y * ctx->width];

        for (int x = p->x0; x <= p->x1; x++) {
            float b3 = 1.0f - b1 - b2;
            if (b1 >= 0 && b2 >= 0 && b3 >= 0) {
                float z = b1*p->v1.z + b2*p->v2.z + b3*p->v3.z;
                if (!ctx->depth_test_enabled || z < line_depth[x]) {
                    float iw = b1*p->iw1 + b2*p->iw2 + b3*p->iw3;
                    float w = (iw != 0) ? 1.0f / iw : 0;
                    float u = (b1*(p->t1.x*p->iw1) + b2*(p->t2.x*p->iw2) + b3*(p->t3.x*p->iw3)) * w;
                    float v_ = (b1*(p->t1.y*p->iw1) + b2*(p->t2.y*p->iw2) + b3*(p->t3.y*p->iw3)) * w;
                    float sh = (b1*(p->t1.z*p->iw1) + b2*(p->t2.z*p->iw2) + b3*(p->t3.z*p->iw3)) * w;
                    if (p->t1.z == 0 && p->t2.z == 0 && p->t3.z == 0) sh = 1.0f;

                    uint32_t color;
                    if (ctx->texture) {
                        int tx = _umod((int)(u * (float)ctx->tex_width),  ctx->tex_width);
                        int ty = _umod((int)(v_ * (float)ctx->tex_height), ctx->tex_height);
                        color = ctx->texture[ty * ctx->tex_width + tx];
                        if ((color >> 24) < 128) goto next_pixel;
                    } else color = ctx->current_color;

                    if (sh != 1.0f) {
                        uint8_t r = (uint8_t)((float)((color>>16)&0xFF) * sh);
                        uint8_t g = (uint8_t)((float)((color>> 8)&0xFF) * sh);
                        uint8_t b = (uint8_t)((float)( color    &0xFF) * sh);
                        color = 0xFF000000u | (r << 16) | (g << 8) | b;
                    }
                    line_depth[x] = z;
                    line_color[x] = color;
                }
            }
        next_pixel:
            b1 += p->b1_x_step;
            b2 += p->b2_x_step;
        }
        b1_row_start += p->b1_y_step;
        b2_row_start += p->b2_y_step;
    }
}

void gl_draw_triangle(gl_context_t *ctx, vec3_t v1, vec3_t v2, vec3_t v3, vec3_t t1, vec3_t t2, vec3_t t3, float iw1, float iw2, float iw3) {
    int x0 = (int)_fmax(0, _fmin(v1.x, _fmin(v2.x, v3.x))), x1 = (int)_fmin(ctx->width-1, _fmax(v1.x, _fmax(v2.x, v3.x)));
    int y0 = (int)_fmax(0, _fmin(v1.y, _fmin(v2.y, v3.y))), y1 = (int)_fmin(ctx->height-1, _fmax(v1.y, _fmax(v2.y, v3.y)));
    if (x1 < x0 || y1 < y0) return;

    float den = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);
    if (_fabs(den) < 1e-6f) return;
    float inv_den = 1.0f / den;

    triangle_task_t p;
    p.ctx = ctx;
    p.v1 = v1; p.v2 = v2; p.v3 = v3;
    p.t1 = t1; p.t2 = t2; p.t3 = t3;
    p.iw1 = iw1; p.iw2 = iw2; p.iw3 = iw3;
    p.inv_den = inv_den;
    p.b1_x_step = (v2.y - v3.y) * inv_den;
    p.b1_y_step = (v3.x - v2.x) * inv_den;
    p.b2_x_step = (v3.y - v1.y) * inv_den;
    p.b2_y_step = (v1.x - v3.x) * inv_den;

    float start_px = (float)x0 + 0.5f;
    float start_py = (float)y0 + 0.5f;
    p.b1_row_start0 = ((v2.y - v3.y) * (start_px - v3.x) + (v3.x - v2.x) * (start_py - v3.y)) * inv_den;
    p.b2_row_start0 = ((v3.y - v1.y) * (start_px - v3.x) + (v1.x - v3.x) * (start_py - v3.y)) * inv_den;
    p.x0 = x0; p.x1 = x1; p.y0 = y0;

    smp_parallel_for(parallel_triangle_task, &p, y1 - y0 + 1);
}


typedef struct { vec4_t p; float u, v, s; } _cv_t;
static int _clip_near(const _cv_t *in, int n, _cv_t *out) {
    int on = 0; const float EPS = 0.001f;
    for (int i=0; i<n; i++) {
        const _cv_t *c = &in[i], *nx = &in[(i+1)%n];
        int cin = (c->p.w >= EPS), nin = (nx->p.w >= EPS);
        if (cin) {
            out[on++] = *c;
            if (!nin) {
                float t = (EPS - c->p.w) / (nx->p.w - c->p.w);
                out[on].p.x = c->p.x + t*(nx->p.x-c->p.x); out[on].p.y = c->p.y + t*(nx->p.y-c->p.y);
                out[on].p.z = c->p.z + t*(nx->p.z-c->p.z); out[on].p.w = EPS;
                out[on].u = c->u + t*(nx->u-c->u); out[on].v = c->v + t*(nx->v-c->v); out[on].s = c->s + t*(nx->s-c->s);
                on++;
            }
        } else if (nin) {
            float t = (EPS - c->p.w) / (nx->p.w - c->p.w);
            out[on].p.x = c->p.x + t*(nx->p.x-c->p.x); out[on].p.y = c->p.y + t*(nx->p.y-c->p.y);
            out[on].p.z = c->p.z + t*(nx->p.z-c->p.z); out[on].p.w = EPS;
            out[on].u = c->u + t*(nx->u-c->u); out[on].v = c->v + t*(nx->v-c->v); out[on].s = c->s + t*(nx->s-c->s);
            on++;
        }
    }
    return on;
}

int gl_draw_triangle_3d(gl_context_t *ctx, vec3_t w1, vec3_t w2, vec3_t w3, vec3_t t1, vec3_t t2, vec3_t t3) {
    mat4_t mv, mvp; mat4_mul(&mv, &ctx->view, &ctx->model); mat4_mul(&mvp, &ctx->projection, &mv);
    _cv_t poly[8], clipped[8];
    poly[0] = (_cv_t){ mat4_mul_vec4(&mvp, (vec4_t){w1.x,w1.y,w1.z,1}), t1.x, t1.y, 1.0f };
    poly[1] = (_cv_t){ mat4_mul_vec4(&mvp, (vec4_t){w2.x,w2.y,w2.z,1}), t2.x, t2.y, 1.0f };
    poly[2] = (_cv_t){ mat4_mul_vec4(&mvp, (vec4_t){w3.x,w3.y,w3.z,1}), t3.x, t3.y, 1.0f };
    int n = _clip_near(poly, 3, clipped); if (n < 3) return 0;
    vec3_t sv[8]; float siw[8];
    for (int i=0; i<n; i++) {
        float iw = 1.0f / clipped[i].p.w; siw[i] = iw;
        sv[i].x = (clipped[i].p.x * iw + 1.0f) * 0.5f * (float)(ctx->width-1);
        sv[i].y = (1.0f - clipped[i].p.y * iw) * 0.5f * (float)(ctx->height-1);
        sv[i].z = clipped[i].p.z * iw;
    }
    int drawn = 0;
    for (int i=1; i<n-1; i++) {
        int a=0, b=i, c=i+1;
        if (ctx->cull_backface) {
            float area = (sv[b].x-sv[a].x)*(sv[c].y-sv[a].y) - (sv[b].y-sv[a].y)*(sv[c].x-sv[a].x);
            if (area <= 0) continue;
        }
        gl_draw_triangle(ctx, sv[a], sv[b], sv[c], (vec3_t){clipped[a].u,clipped[a].v,clipped[a].s}, (vec3_t){clipped[b].u,clipped[b].v,clipped[b].s}, (vec3_t){clipped[c].u,clipped[c].v,clipped[c].s}, siw[a], siw[b], siw[c]);
        drawn++;
    }
    return drawn;
}

void gl_draw_line(gl_context_t *ctx, vec3_t a, vec3_t b) {
    mat4_t mv, mvp; mat4_mul(&mv, &ctx->view, &ctx->model); mat4_mul(&mvp, &ctx->projection, &mv);
    vec4_t c1 = mat4_mul_vec4(&mvp, (vec4_t){a.x,a.y,a.z,1}), c2 = mat4_mul_vec4(&mvp, (vec4_t){b.x,b.y,b.z,1});
    if (c1.w < 0.001f && c2.w < 0.001f) return;
    if (c1.w < 0.001f || c2.w < 0.001f) {
        if (c1.w < 0.001f) { vec4_t t = c1; c1 = c2; c2 = t; }
        float t = (0.001f - c1.w) / (c2.w - c1.w);
        c2.x = c1.x + t*(c2.x-c1.x); c2.y = c1.y + t*(c2.y-c1.y); c2.z = c1.z + t*(c2.z-c1.z); c2.w = 0.001f;
    }
    float i1 = 1.0f / c1.w, i2 = 1.0f / c2.w;
    int x1 = (int)((c1.x*i1+1)*0.5f*(ctx->width-1)), y1 = (int)((1-c1.y*i1)*0.5f*(ctx->height-1));
    int x2 = (int)((c2.x*i2+1)*0.5f*(ctx->width-1)), y2 = (int)((1-c2.y*i2)*0.5f*(ctx->height-1));
    int dx = _fabs(x2-x1), sx = x1<x2?1:-1, dy = -_fabs(y2-y1), sy = y1<y2?1:-1, err = dx+dy, e2;
    for (;;) {
        if ((unsigned)x1<(unsigned)ctx->width && (unsigned)y1<(unsigned)ctx->height) ctx->color_buffer[y1*ctx->width+x1] = ctx->current_color;
        if (x1==x2 && y1==y2) break;
        e2 = 2*err; if (e2>=dy) { err+=dy; x1+=sx; } if (e2<=dx) { err+=dx; y1+=sy; }
    }
}


gl_context_t *g_fgl = NULL;

int fgl_init_gl(int width, int height) {
    if (!g_fgl) g_fgl = (gl_context_t*)kzalloc(sizeof(gl_context_t));
    if (!g_fgl) return -1;
    uint32_t *cb = (uint32_t*)kmalloc((size_t)width*height*4);
    float *db = (float*)kmalloc((size_t)width*height*4);
    gl_init(g_fgl, width, height, cb, db);
    g_fgl->programs[0].used = 1;
    for (int i=0; i<16; i++) g_fgl->programs[0].mat_proj[i] = g_fgl->programs[0].mat_view[i] = (i%5==0)?1.0f:0.0f;
    return 0;
}

void fgl_present(void) {
    if (!g_fgl) return;
    vbe_bb_blit_rect(0, 0, g_fgl->width, g_fgl->height, g_fgl->color_buffer);
    vbe_flip();
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { (void)x; (void)y; if (g_fgl) { g_fgl->width = w; g_fgl->height = h; } }
void glEnable(GLenum cap) { if (!g_fgl) return; if (cap == GL_DEPTH_TEST) g_fgl->depth_test_enabled = 1; if (cap == GL_CULL_FACE) g_fgl->cull_backface = 1; }
void glDisable(GLenum cap) { if (!g_fgl) return; if (cap == GL_DEPTH_TEST) g_fgl->depth_test_enabled = 0; if (cap == GL_CULL_FACE) g_fgl->cull_backface = 0; }
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { (void)a; if (g_fgl) g_fgl->current_color = (0xFF<<24) | ((int)(r*255)<<16) | ((int)(g*255)<<8) | (int)(b*255); }
void glClear(GLbitfield mask) { if (!g_fgl) return; if (mask & GL_COLOR_BUFFER_BIT) gl_clear(g_fgl, g_fgl->current_color); }

void glGenBuffers(GLsizei n, GLuint *bufs) {
    if (!g_fgl) return;
    for (int i=0; i<n; i++) {
        for (int j=1; j<FGL_MAX_BUFFERS; j++) if (!g_fgl->buffers[j].used) {
            g_fgl->buffers[j].used = 1; bufs[i] = j; break;
        }
    }
}
void glBindBuffer(GLenum target, GLuint buf) { if (g_fgl && target == GL_ARRAY_BUFFER) g_fgl->bound_array_buffer = buf; }
void glBufferData(GLenum target, size_t size, const void *data, GLenum usage) {
    (void)target; (void)usage; if (!g_fgl || !g_fgl->bound_array_buffer) return;
    fgl_buffer_t *b = &g_fgl->buffers[g_fgl->bound_array_buffer];
    if (b->data) kfree(b->data); b->data = (float*)kmalloc(size); b->size = size;
    if (data) { uint8_t *d=(uint8_t*)data, *s=(uint8_t*)b->data; for (size_t i=0; i<size; i++) s[i]=d[i]; }
}
void glDeleteBuffers(GLsizei n, const GLuint *bufs) { if (!g_fgl) return; for (int i=0; i<n; i++) if (bufs[i]<FGL_MAX_BUFFERS) { if (g_fgl->buffers[bufs[i]].data) kfree(g_fgl->buffers[bufs[i]].data); g_fgl->buffers[bufs[i]].used=0; } }

void glEnableVertexAttribArray(GLuint idx) { if (g_fgl && idx < FGL_MAX_ATTRIBS) g_fgl->attribs[idx].enabled = 1; }
void glDisableVertexAttribArray(GLuint idx) { if (g_fgl && idx < FGL_MAX_ATTRIBS) g_fgl->attribs[idx].enabled = 0; }
void glVertexAttribPointer(GLuint idx, GLint sz, GLenum type, GLboolean norm, GLsizei stride, size_t off) {
    (void)norm; if (!g_fgl || idx >= FGL_MAX_ATTRIBS) return;
    g_fgl->attribs[idx].buffer = g_fgl->bound_array_buffer; g_fgl->attribs[idx].size = sz;
    g_fgl->attribs[idx].type = type; g_fgl->attribs[idx].stride = stride; g_fgl->attribs[idx].offset = off;
}

void glGenTextures(GLsizei n, GLuint *texs) {
    if (!g_fgl) return;
    for (int i=0; i<n; i++) {
        for (int j=1; j<FGL_MAX_TEXTURES; j++) if (!g_fgl->textures[j].used) {
            g_fgl->textures[j].used = 1; texs[i] = j; break;
        }
    }
}
void glBindTexture(GLenum target, GLuint tex) { if (g_fgl && target == GL_TEXTURE_2D) g_fgl->bound_texture = tex; }
void glTexImage2D(GLenum target, GLint lvl, GLint ifmt, GLsizei w, GLsizei h, GLint b, GLenum fmt, GLenum type, const void *px) {
    (void)target; (void)lvl; (void)ifmt; (void)b; (void)fmt; (void)type;
    if (!g_fgl || !g_fgl->bound_texture) return;
    fgl_texture_t *t = &g_fgl->textures[g_fgl->bound_texture];
    if (t->pixels) kfree(t->pixels); t->pixels = (uint32_t*)kmalloc((size_t)w*h*4); t->width=w; t->height=h;
    if (px) { uint32_t *s=(uint32_t*)px, *d=t->pixels; for (int i=0; i<w*h; i++) d[i]=s[i]; }
}
void glTexParameteri(GLenum target, GLenum pname, GLint p) { (void)target; (void)pname; (void)p; }

static float _fetch(fgl_attrib_t *a, int vidx, int comp) {
    if (!a->enabled || !a->buffer) return 0;
    fgl_buffer_t *b = &g_fgl->buffers[a->buffer];
    size_t stride = a->stride ? a->stride : (size_t)a->size*4;
    float *ptr = (float*)((uint8_t*)b->data + a->offset + (size_t)vidx*stride);
    return ptr[comp];
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!g_fgl || mode != GL_TRIANGLES) return;
    fgl_program_t *p = &g_fgl->programs[0];

    for (int i=0; i<4; i++) for (int j=0; j<4; j++) {
        g_fgl->projection.m[i][j] = p->mat_proj[j*4+i];
        g_fgl->view.m[i][j] = p->mat_view[j*4+i];
        g_fgl->model.m[i][j] = (i==j)?1.0f:0.0f;
    }
    if (g_fgl->bound_texture) {
        g_fgl->texture = g_fgl->textures[g_fgl->bound_texture].pixels;
        g_fgl->tex_width = g_fgl->textures[g_fgl->bound_texture].width;
        g_fgl->tex_height = g_fgl->textures[g_fgl->bound_texture].height;
    } else g_fgl->texture = NULL;

    for (int i=0; i<count; i+=3) {
        vec3_t v[3], t[3];
        for (int j=0; j<3; j++) {
            int vidx = first + i + j;
            v[j].x = _fetch(&g_fgl->attribs[FGL_ATTRIB_POSITION], vidx, 0);
            v[j].y = _fetch(&g_fgl->attribs[FGL_ATTRIB_POSITION], vidx, 1);
            v[j].z = _fetch(&g_fgl->attribs[FGL_ATTRIB_POSITION], vidx, 2);
            t[j].x = _fetch(&g_fgl->attribs[FGL_ATTRIB_TEXCOORD], vidx, 0);
            t[j].y = _fetch(&g_fgl->attribs[FGL_ATTRIB_TEXCOORD], vidx, 1);
            t[j].z = _fetch(&g_fgl->attribs[FGL_ATTRIB_SHADE], vidx, 0);
        }
        gl_draw_triangle_3d(g_fgl, v[0], v[1], v[2], t[0], t[1], t[2]);
    }
}

GLuint glCreateProgram(void) { return 0; }
void glUseProgram(GLuint p) { (void)p; }
GLint glGetUniformLocation(GLuint p, const char *n) {
    (void)p; if (n[0]=='u' && n[1]=='P') return FGL_UNIFORM_PROJECTION;
    if (n[0]=='u' && n[1]=='V') return FGL_UNIFORM_VIEW;
    if (n[0]=='u' && n[1]=='T') return FGL_UNIFORM_TEXTURE;
    return -1;
}
void glUniformMatrix4fv(GLint loc, GLsizei c, GLboolean t, const GLfloat *v) {
    (void)c; (void)t; if (!g_fgl || loc < 0) return;
    float *dst = (loc == FGL_UNIFORM_PROJECTION) ? g_fgl->programs[0].mat_proj : g_fgl->programs[0].mat_view;
    for (int i=0; i<16; i++) dst[i] = v[i];
}
void glUniform1i(GLint loc, GLint v) { (void)loc; (void)v; }




