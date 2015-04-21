#ifndef GLXWORKER_H
#define GLXWORKER_H

#include "commonincludes.hpp"
class GLXWorker {
    public:
        static void initGLX();
        
        static void destroyContext(Window win);
        static void endFrame(Window win);

        static void createNewContext(Window win);
        static void createDefaultContext(Window win);

        static GLuint loadImage(char *path);
        static GLuint loadShader(const char *path, GLuint type);
        static GLuint compileShader(const char* src, GLuint type);
        static GLuint textureFromPixmap(Pixmap pixmap, int, int, XVisualInfo*);

    private:
        static void initFBConf();
        static GLXPixmap glxPixmap(Pixmap pixmap, GLXFBConfig, int, int);
        static GLXFBConfig fbconfigs[33];
};
#endif
