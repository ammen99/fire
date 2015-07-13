#ifndef GLXWORKER_H
#define GLXWORKER_H

#include "commonincludes.hpp"
class Core;
struct SharedImage;
namespace GLXUtils {
    void initGLX(Core *core);

    void destroyContext(Window win);
    void endFrame(Window win);

    void createNewContext(Window win);
    void createDefaultContext(Window win);
    Window createNewWindowWithContext(Window parent, Core *core);

    GLuint loadImage(char *path);
    GLuint loadShader(const char *path, GLuint type);
    GLuint compileShader(const char* src, GLuint type);
    GLuint textureFromPixmap(Pixmap pixmap, int w, int h, SharedImage *sh);

    void initFBConf(Core *core);
    GLXPixmap glxPixmap(Pixmap pixmap, GLXFBConfig, int, int);

    extern GLXFBConfig fbconfigs[33];
};
#endif
