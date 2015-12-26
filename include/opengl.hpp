#ifndef DRIVER_H
#define DRIVER_H
#include "core.hpp"

namespace OpenGL {
    struct _API {
        PFNGLACTIVETEXTUREPROC glActiveTexture;
        PFNGLATTACHSHADERPROC glAttachShader;
        PFNGLBINDBUFFERPROC glBindBuffer;
        PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
        PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
        //PFNGLBINDTEXTUREPROC BindTexture;
        PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
        //PFNGLBLENDFUNCPROC BlendFunc;
        PFNGLBUFFERDATAPROC glBufferData;
        //PFNGLCLEARPROC Clear;
        //PFNGLCLEARCOLORPROC ClearColor;
        //PFNGLCLEARDEPTHPROC ClearDepth;
        PFNGLCOMPILESHADERPROC glCompileShader;
        PFNGLCREATEPROGRAMPROC glCreateProgram;
        PFNGLCREATESHADERPROC glCreateShader;
        PFNGLDELETEBUFFERSPROC glDeleteBuffers;
        //PFNGLDELETETEXTURESPROC DeleteTextures;
        PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
        //PFNGLDISABLEPROC Disable;
        //PFNGLDRAWARRAYSPROC DrawArrays;
        //PFNGLENABLEPROC Enable;
        PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
        //PFNGLFINISHPROC Finish;
        PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
        PFNGLGENBUFFERSPROC glGenBuffers;
        PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
        //PFNGLGENTEXTURESPROC GenTextures;
        PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
        PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
        PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
        PFNGLGETSHADERIVPROC glGetShaderiv;
        PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
        PFNGLLINKPROGRAMPROC glLinkProgram;
        //PFNGLPIXELSTOREPROC PixelStorei;
        //PFNGLREADPIXELSPROC ReadPixels;
        //PFNGLSCISSORPROC Scissor;
        PFNGLSHADERSOURCEPROC glShaderSource;
        //PFNGLTEXIMAGE2DPROC TexImage2D;
        //PFNGLTEXPARAMETERIPROC TexParameteri;
        PFNGLUNIFORM1FPROC glUniform1f;
        PFNGLUNIFORM1IPROC glUniform1i;
        PFNGLUNIFORM4FVPROC glUniform4fv;
        PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
        PFNGLUSEPROGRAMPROC glUseProgram;
        PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
        //PFNGLVIEWPORTPROC Viewport;
        
        PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
        PFNGLDELETETEXTURESEXTPROC glDeleteTextures;
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
