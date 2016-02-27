#ifndef DRIVER_H
#define DRIVER_H

#include "core.hpp"

namespace OpenGL {
    extern bool      transformed;
    extern int       depth;
    extern glm::vec4 color;

    extern int VersionMinor, VersionMajor;

    void initOpenGL(const char *shaderSrcPath);
    void renderTransformedTexture(GLuint text, const wlc_geometry& g, glm::mat4 transform);
    void renderTexture(GLuint text, const wlc_geometry& g);

    void preStage();
    void preStage(GLuint fbuff);
    void endStage();

    void prepareFramebuffer(GLuint &fbuff, GLuint &texture);
    GLuint getTex();

    /* set program to current program */
    void useDefaultProgram();

    /* reset OpenGL state */
    void reset_gl();
}


#endif
