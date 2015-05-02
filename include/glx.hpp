#ifndef GLXWORKER_H
#define GLXWORKER_H

#include "commonincludes.hpp"
namespace GLXUtils {
    void initGLX();

    void destroyContext(Window win);
    void endFrame(Window win);

    void createNewContext(Window win);
    void createDefaultContext(Window win);

    GLuint loadImage(char *path);
    GLuint loadShader(const char *path, GLuint type);
    GLuint compileShader(const char* src, GLuint type);
    GLuint textureFromPixmap(Pixmap pixmap, int, int, XVisualInfo*);

    void initFBConf();
    GLXPixmap glxPixmap(Pixmap pixmap, GLXFBConfig, int, int);

    extern GLXFBConfig fbconfigs[33];
};
#endif
