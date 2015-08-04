#ifndef GLXWORKER_H
#define GLXWORKER_H

#include "commonincludes.hpp"
class Core;
struct SharedImage;
namespace GLXUtils {
    void initGLX();

    void destroyContext(Window win);
    Window createNewWindowWithContext(Window parent);

    GLuint loadImage(char *path);
    GLuint loadShader(const char *path, GLuint type);
    GLuint compileShader(const char* src, GLuint type);
    GLuint textureFromPixmap(Pixmap pixmap, int w, int h, SharedImage *sh);
}
#endif
