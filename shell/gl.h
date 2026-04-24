/* scorpion OS - shell/gl.h
   ScorpionGL — software WebGL-like 3D renderer
   Runs entirely on CPU, outputs to VBE framebuffer.

   API is intentionally WebGL/OpenGL-ES flavoured so porting
   browser WebGL demos is straightforward.

   Supports:
   - Perspective-correct triangle rasterisation
   - Depth (Z) buffer
   - Nearest-neighbour texture sampling (power-of-two textures)
   - Per-face shading (shade multiplier on vertex colour)
   - Flat and Gouraud colour interpolation
   - Matrix stack (PROJECTION / MODELVIEW)
   - Vertex buffer objects (upload once, draw many times)
   - glDrawArrays (GL_TRIANGLES only — enough for voxels)
   - glClear, glViewport, glEnable/Disable DEPTH_TEST, CULL_FACE
   - glUniformMatrix4fv, glUniform1i
   - glVertexAttribPointer (stride/offset)
   - glTexImage2D, glTexParameter (NEAREST filter)
   - Software blending (none — alpha discard at 128)              */

#ifndef SCORPION_GL_H
#define SCORPION_GL_H

#include <stdint.h>
#include <stddef.h>

/* ---- Types ---- */
typedef float     GLfloat;
typedef int       GLint;
typedef unsigned  GLuint;
typedef int       GLsizei;
typedef uint8_t   GLubyte;
typedef unsigned  GLenum;
typedef unsigned  GLbitfield;
typedef int       GLboolean;

/* ---- Constants (matching WebGL/OpenGL-ES values) ---- */
#define GL_TRIANGLES            0x0004
#define GL_DEPTH_TEST           0x0B71
#define GL_CULL_FACE            0x0B44
#define GL_TEXTURE_2D           0x0DE1
#define GL_RGBA                 0x1908
#define GL_RGB                  0x1907
#define GL_UNSIGNED_BYTE        0x1401
#define GL_FLOAT                0x1406
#define GL_NEAREST              0x2600
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_REPEAT               0x2901
#define GL_COLOR_BUFFER_BIT     0x4000
#define GL_DEPTH_BUFFER_BIT     0x0100
#define GL_ARRAY_BUFFER         0x8892
#define GL_STATIC_DRAW          0x88B4
#define GL_VERTEX_SHADER        0x8B31
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_COMPILE_STATUS       0x8B81
#define GL_LINK_STATUS          0x8B82
#define GL_TRUE                 1
#define GL_FALSE                0
#define GL_NONE                 0

/* Max resources */
#define SGL_MAX_TEXTURES    32
#define SGL_MAX_BUFFERS     64
#define SGL_MAX_ATTRIBS      8
#define SGL_MAX_UNIFORMS    16

/* ---- Texture ---- */
typedef struct {
    int       used;
    int       width, height;
    uint32_t *pixels;   /* ARGB packed */
    GLenum    wrap_s, wrap_t;
    GLenum    min_filter, mag_filter;
} sgl_texture_t;

/* ---- Buffer ---- */
typedef struct {
    int     used;
    float  *data;
    size_t  size;   /* bytes */
} sgl_buffer_t;

/* ---- Vertex attribute binding ---- */
typedef struct {
    int    enabled;
    GLuint buffer;
    GLint  size;        /* components: 1/2/3/4 */
    GLenum type;        /* GL_FLOAT */
    GLsizei stride;
    size_t  offset;
} sgl_attrib_t;

/* ---- Uniform ---- */
typedef enum { SGL_UNI_NONE, SGL_UNI_MAT4, SGL_UNI_INT, SGL_UNI_FLOAT } sgl_uni_type_t;
typedef struct {
    sgl_uni_type_t type;
    char  name[32];
    float mat[16];
    int   ival;
    float fval;
} sgl_uniform_t;

/* ---- Program (stub — we hard-code the voxel shader pipeline) ---- */
typedef struct {
    int           used;
    sgl_uniform_t uniforms[SGL_MAX_UNIFORMS];
    int           n_uniforms;
} sgl_program_t;

/* ---- ScorpionGL context ---- */
typedef struct {
    /* Framebuffer */
    uint32_t  *color_buf;   /* ARGB */
    float     *depth_buf;   /* 1/w per pixel */
    int        width, height;

    /* State */
    int        depth_test;
    int        cull_face;
    uint32_t   clear_color;
    float      clear_depth;

    /* Current program (always 0 for us) */
    sgl_program_t programs[1];
    GLuint        cur_program;

    /* Textures */
    sgl_texture_t textures[SGL_MAX_TEXTURES];
    GLuint        bound_texture;

    /* Buffers */
    sgl_buffer_t  buffers[SGL_MAX_BUFFERS];
    GLuint        bound_array_buffer;

    /* Vertex attribs */
    sgl_attrib_t  attribs[SGL_MAX_ATTRIBS];

    /* Matrices (set via glUniformMatrix4fv) */
    float         proj[16];
    float         view[16];

} sgl_ctx_t;

/* ---- Global context (single) ---- */
extern sgl_ctx_t *g_sgl;

/* ---- API ---- */

/* Init/destroy */
int  sgl_init(int width, int height);
void sgl_destroy(void);
/* Blit colour buffer to VBE framebuffer */
void sgl_present(void);

/* Core GL functions */
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glClear(GLbitfield mask);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

/* Shaders/programs (stub — always succeed, voxel pipeline is hard-coded) */
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

/* Buffers */
void glGenBuffers(GLsizei n, GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, size_t size, const void *data, GLenum usage);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);

/* Vertex attributes */
void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, size_t offset);

/* Textures */
void glGenTextures(GLsizei n, GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glDeleteTextures(GLsizei n, const GLuint *textures);

/* Draw */
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

/* Named attrib indices for voxel renderer (matching the GLSL names in the demo) */
#define SGL_ATTRIB_POSITION  0
#define SGL_ATTRIB_TEXCOORD  1
#define SGL_ATTRIB_SHADE     2

/* Named uniform indices */
#define SGL_UNIFORM_PROJECTION 0
#define SGL_UNIFORM_VIEW       1
#define SGL_UNIFORM_TEXTURE    2

/* Shader log stubs */
void glGetShaderInfoLog(GLuint shader, GLsizei maxLen, GLsizei *length, char *buf);
void glGetProgramInfoLog(GLuint program, GLsizei maxLen, GLsizei *length, char *buf);

#endif /* SCORPION_GL_H */
