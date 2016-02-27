#ifndef GLXWORKER_H
#define GLXWORKER_H

#include "commonincludes.hpp"
class Core;
namespace GLXUtils {
    GLuint loadShader(const char *path, GLuint type);
    GLuint compileShader(const char* src, GLuint type);
}
#endif
