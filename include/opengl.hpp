#ifndef DRIVER_H
#define DRIVER_H
#include "core.hpp"

namespace OpenGL {
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
