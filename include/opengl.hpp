#ifndef DRIVER_H
#define DRIVER_H
#include "core.hpp"

namespace OpenGL {
    struct _API {
        PFNGLACTIVETEXTUREPROC glActiveTexture;
        PFNGLATTACHSHADERPROC glAttachShader;
        PFNGLBINDBUFFERPROC glBindBuffer;
        PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
        PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
        PFNGLBUFFERDATAPROC glBufferData;
        PFNGLCOMPILESHADERPROC glCompileShader;
        PFNGLCREATEPROGRAMPROC glCreateProgram;
        PFNGLCREATESHADERPROC glCreateShader;
        PFNGLDELETEBUFFERSPROC glDeleteBuffers;
        PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
        PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
        PFNGLGENBUFFERSPROC glGenBuffers;
        PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
        PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
        PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
        PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
        PFNGLGETSHADERIVPROC glGetShaderiv;
        PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
        PFNGLLINKPROGRAMPROC glLinkProgram;
        PFNGLSHADERSOURCEPROC glShaderSource;
        PFNGLUNIFORM1FPROC glUniform1f;
        PFNGLUNIFORM1IPROC glUniform1i;
        PFNGLUNIFORM4FVPROC glUniform4fv;
        PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
        PFNGLUSEPROGRAMPROC glUseProgram;
        PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

        PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
        PFNGLDELETETEXTURESEXTPROC glDeleteTextures;
        PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
    };

    extern _API API;



    extern bool      transformed;
    extern int       depth;
    extern glm::vec4 color;

    extern int VersionMinor, VersionMajor;

    void initOpenGL(const char *shaderSrcPath);
    void renderTransformedTexture(GLuint text, GLuint vao,
     GLuint vbo, glm::mat4 t);
    void renderTexture(GLuint text, GLuint vao, GLuint vbo);

    void preStage();
    void preStage(GLuint fbuff);
    void endStage();

    void generateVAOVBO(int, int, int, int, GLuint&, GLuint&);
    void generateVAOVBO(int, int, GLuint&, GLuint&);

    void prepareFramebuffer(GLuint &fbuff, GLuint &texture);
    GLuint getTex();
    /* set program to current program */
    void useDefaultProgram();
}
#endif
