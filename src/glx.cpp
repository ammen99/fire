#include "core.hpp"
#include "opengl.hpp"

namespace GLXUtils {
#define uchar unsigned char
GLuint compileShader(const char *src, GLuint type) {
    printf("compile shader\n");

    GLuint shader = glCreateShader(type);
    glShaderSource ( shader, 1, &src, NULL );

    int s;
    char b1[10000];
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &s);
    glGetShaderInfoLog(shader, 10000, NULL, b1);


    if ( s == GL_FALSE ) {
        printf("shader compilation failed!\n"
               "src: ***************************\n"
               "%s\n"
               "********************************\n"
               "%s\n"
               "********************************\n", src, b1);
        return -1;
    }
    return shader;
}

GLuint loadShader(const char *path, GLuint type) {

    std::fstream file(path, std::ios::in);
    if(!file.is_open())
        printf("Cannot open shader file %s. Aborting\n", path),
            std::exit(1);

    std::string str, line;

    while(std::getline(file, line))
        str += line, str += '\n';

    return compileShader(str.c_str(), type);
}
}

