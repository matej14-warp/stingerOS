








typedef float     GLfloat;
typedef int       GLint;
typedef unsigned  GLuint;
typedef int       GLsizei;
typedef uint8_t   GLubyte;
typedef unsigned  GLenum;
typedef unsigned  GLbitfield;
typedef int       GLboolean;




































typedef struct {
    int       used;
    int       width, height;
    uint32_t *pixels;
    GLenum    wrap_s, wrap_t;
    GLenum    min_filter, mag_filter;
} sgl_texture_t;


typedef struct {
    int     used;
    float  *data;
    size_t  size;
} sgl_buffer_t;


typedef struct {
    int    enabled;
    GLuint buffer;
    GLint  size;
    GLenum type;
    GLsizei stride;
    size_t  offset;
} sgl_attrib_t;


typedef enum { SGL_UNI_NONE, SGL_UNI_MAT4, SGL_UNI_INT, SGL_UNI_FLOAT } sgl_uni_type_t;
typedef struct {
    sgl_uni_type_t type;
    char  name[32];
    float mat[16];
    int   ival;
    float fval;
} sgl_uniform_t;


typedef struct {
    int           used;
    sgl_uniform_t uniforms[SGL_MAX_UNIFORMS];
    int           n_uniforms;
} sgl_program_t;


typedef struct {

    uint32_t  *color_buf;
    float     *depth_buf;
    int        width, height;


    int        depth_test;
    int        cull_face;
    uint32_t   clear_color;
    float      clear_depth;


    sgl_program_t programs[1];
    GLuint        cur_program;


    sgl_texture_t textures[SGL_MAX_TEXTURES];
    GLuint        bound_texture;


    sgl_buffer_t  buffers[SGL_MAX_BUFFERS];
    GLuint        bound_array_buffer;


    sgl_attrib_t  attribs[SGL_MAX_ATTRIBS];


    float         proj[16];
    float         view[16];

} sgl_ctx_t;


extern sgl_ctx_t *g_sgl;




int  sgl_init(int width, int height);
void sgl_destroy(void);

void sgl_present(void);


void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glClear(GLbitfield mask);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);


GLuint glCreateShader(GLenum type);
void   glShaderSource(GLuint shader, GLsizei count, const char **strings, const GLint *lengths);
void   glCompileShader(GLuint shader);
void   glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
void   glDeleteShader(GLuint shader);
GLuint glCreateProgram(void);
void   glAttachShader(GLuint program, GLuint shader);
void   glLinkProgram(GLuint program);
void   glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void   glUseProgram(GLuint program);
GLint  glGetAttribLocation(GLuint program, const char *name);
GLint  glGetUniformLocation(GLuint program, const char *name);
void   glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void   glUniform1i(GLint location, GLint v0);


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
void glDeleteTextures(GLsizei n, const GLuint *textures);


void glDrawArrays(GLenum mode, GLint first, GLsizei count);












void glGetShaderInfoLog(GLuint shader, GLsizei maxLen, GLsizei *length, char *buf);
void glGetProgramInfoLog(GLuint program, GLsizei maxLen, GLsizei *length, char *buf);






