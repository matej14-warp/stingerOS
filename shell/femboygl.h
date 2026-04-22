








typedef float     GLfloat;
typedef int       GLint;
typedef unsigned  GLuint;
typedef int       GLsizei;
typedef uint8_t   GLubyte;
typedef unsigned  GLenum;
typedef unsigned  GLbitfield;
typedef int       GLboolean;























typedef struct { float x, y, z;    } vec3_t;
typedef struct { float x, y, z, w; } vec4_t;
typedef struct { float m[4][4];    } mat4_t;








typedef struct {
    int       used;
    int       width, height;
    uint32_t *pixels;
    GLenum    wrap_s, wrap_t;
} fgl_texture_t;

typedef struct {
    int     used;
    float  *data;
    size_t  size;
} fgl_buffer_t;

typedef struct {
    int    enabled;
    GLuint buffer;
    GLint  size;
    GLenum type;
    GLsizei stride;
    size_t  offset;
} fgl_attrib_t;

typedef struct {
    int    used;
    float  mat_proj[16];
    float  mat_view[16];
} fgl_program_t;


typedef struct {

    uint32_t *color_buffer;
    float    *depth_buffer;
    int       width, height;


    mat4_t model, view, projection;


    uint32_t  current_color;
    uint32_t *texture;
    int       tex_width, tex_height;
    int       cull_backface;
    int       depth_test_enabled;


    fgl_texture_t textures[FGL_MAX_TEXTURES];
    GLuint        bound_texture;
    fgl_buffer_t  buffers[FGL_MAX_BUFFERS];
    GLuint        bound_array_buffer;
    fgl_attrib_t  attribs[FGL_MAX_ATTRIBS];
    fgl_program_t programs[1];
    GLuint        cur_program;
} gl_context_t;


void gl_init         (gl_context_t *ctx, int w, int h, uint32_t *color_buf, float *depth_buf);
void gl_clear        (gl_context_t *ctx, uint32_t color);
void gl_load_identity(gl_context_t *ctx);
void gl_set_color    (gl_context_t *ctx, uint32_t color);
void gl_set_texture  (gl_context_t *ctx, uint32_t *tex, int w, int h);
void gl_set_culling  (gl_context_t *ctx, int enable);

void gl_perspective  (gl_context_t *ctx, float fov, float aspect, float near, float far);
void gl_ortho        (gl_context_t *ctx, float l, float r, float b, float t, float n, float f);
void gl_look_at      (gl_context_t *ctx, vec3_t eye, vec3_t center, vec3_t up);
void gl_model_mul    (gl_context_t *ctx, const mat4_t *m);

void gl_draw_line    (gl_context_t *ctx, vec3_t a, vec3_t b);
int  gl_draw_triangle_3d(gl_context_t *ctx, vec3_t w1, vec3_t w2, vec3_t w3, vec3_t t1, vec3_t t2, vec3_t t3);



extern gl_context_t *g_fgl;

int  fgl_init_gl(int width, int height);
void fgl_present(void);

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClear(GLbitfield mask);

void glGenBuffers(GLsizei n, GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, size_t size, const void *data, GLenum usage);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);

void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, size_t offset);

void glGenTextures(GLsizei n, GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);

void glDrawArrays(GLenum mode, GLint first, GLsizei count);











GLuint glCreateProgram(void);
void   glUseProgram(GLuint program);
GLint  glGetUniformLocation(GLuint program, const char *name);
void   glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void   glUniform1i(GLint location, GLint v0);


float _fabs (float a);
float _fmin (float a, float b);
float _fmax (float a, float b);
float _fsqrt(float x);
float _fsin (float x);
float _fcos (float x);
float _ftan (float x);






