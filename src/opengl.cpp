#include "opengl.hpp"

namespace {
    GLuint program;
    GLuint mvpID;
    GLuint depthID, colorID, bgraID;
    glm::mat4 View;
    glm::mat4 Proj;
    glm::mat4 MVP;

    GLuint framebuffer;
    GLuint framebufferTexture;

    GLuint fullVAO, fullVBO;
}

bool OpenGL::transformed = false;
int  OpenGL::depth;
glm::vec4 OpenGL::color;

const char* gl_error_string(const GLenum error) {
    switch (error) {
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
    }

    return "UNKNOWN GL ERROR";
}


void gl_call(const char *func, uint32_t line, const char *glfunc) {
    GLenum error;
    if ((error = glGetError()) == GL_NO_ERROR)
        return;

    printf("gles2: function %s at line %u: %s == %s\n", func, line, glfunc, gl_error_string(error));
}

#ifndef __STRING
#  define __STRING(x) #x
#endif

#define GL_CALL(x) x; gl_call(__PRETTY_FUNCTION__, __LINE__, __STRING(x))


namespace OpenGL {
    int VersionMinor, VersionMajor;

    GLuint getTex() {return framebufferTexture;}

    void renderTexture(GLuint tex, const wlc_geometry& g) {


        float w2 = float(1366) / 2.;
        float h2 = float(768) / 2.;

        float tlx = float(g.origin.x) - w2,
              tly = h2 - float(g.origin.y);

        float w = g.size.w;
        float h = g.size.h;

        GLfloat vertexData[] = {
            tlx    , tly - h, 0.f, // 1
            tlx + w, tly - h, 0.f, // 2
            tlx + w, tly    , 0.f, // 3
            tlx + w, tly    , 0.f, // 3
            tlx    , tly    , 0.f, // 4
            tlx    , tly - h, 0.f, // 1
        };

        GLfloat coordData[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 1.0f,
        };

        glUseProgram(program);

        GLint position   = GL_CALL(glGetAttribLocation(program, "position"));
        GLint uvPosition = GL_CALL(glGetAttribLocation(program, "uvPosition"));
 //       GLint texID      = GL_CALL(glGetAttribLocation(program, "smp"));

        std::cout << position << " " << uvPosition << std::endl;
        auto w2ID = GL_CALL(glGetUniformLocation(program, "w2"));
        auto h2ID = GL_CALL(glGetUniformLocation(program, "h2"));

        glUniform1f(w2ID, 1366. / 2);
        glUniform1f(h2ID, 768. / 2);

        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, tex));
//        GL_CALL(glUniform1i(texID, 0));

        GL_CALL(glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, vertexData));
        GL_CALL(glVertexAttribPointer(uvPosition, 2, GL_FLOAT, GL_FALSE, 0, coordData));

        GL_CALL(glDrawArrays (GL_TRIANGLES, 0, 6));
    }

    void renderTransformedTexture(GLuint tex, const wlc_geometry& g, glm::mat4 Model) {
        if(transformed)
            MVP = Proj * View * Model;
        else
            MVP = Model;

        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
        glUniform1i(depthID, depth);
        glUniform4fv(colorID, 1, &color[0]);

        renderTexture(tex, g);
    }

    void preStage() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    }

    void preStage(GLuint fbuff) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbuff);
        GetTuple(sw, sh, core->getScreenSize());
        glScissor(0, 0, sw, sh);
    }

    void endStage() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        auto tmp = glm::mat4();
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &tmp[0][0]);
        glUniform1i(bgraID, 1);

        OpenGL::color = glm::vec4(1, 1, 1, 1);
        glUniform4fv(colorID, 1, &color[0]);

        GetTuple(sw, sh, core->getScreenSize());
        glScissor(0, 0, sw, sh);


        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1i(bgraID, 0);
    }

    void prepareFramebuffer(GLuint &fbuff, GLuint &texture) {
        GetTuple(sw, sh, core->getScreenSize());

        glGenFramebuffers(1, &fbuff);
        glBindFramebuffer(GL_FRAMEBUFFER, fbuff);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sw, sh,
                0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                texture, 0);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
            std::cout << "Error in framebuffer !!!" << std::endl;
        }
    }

    void useDefaultProgram() {
        GL_CALL(glUseProgram(program));
    }

    void initOpenGL(const char *shaderSrcPath) {

        //glEnable(GL_DEBUG_OUTPUT);
        //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        //glDebugMessageCallback(errorHandler, (void*)0);

        GetTuple(sw, sh, core->getScreenSize());
        std::string tmp = shaderSrcPath;

        GLuint vss =
            GLXUtils::loadShader(std::string(shaderSrcPath)
                    .append("/vertex.glsl").c_str(),
                    GL_VERTEX_SHADER);

        GLuint fss =
            GLXUtils::loadShader(std::string(shaderSrcPath)
                    .append("/frag.glsl").c_str(),
                    GL_FRAGMENT_SHADER);

        program = glCreateProgram();

        std::cout << "ccc" << std::endl;

        GL_CALL(glAttachShader (program, vss));
        GL_CALL(glAttachShader (program, fss));
        GL_CALL(glLinkProgram (program));
        useDefaultProgram();

        mvpID   = GL_CALL(glGetUniformLocation(program, "MVP"));
        depthID = GL_CALL(glGetUniformLocation(program, "depth"));
        colorID = GL_CALL(glGetUniformLocation(program, "color"));
        bgraID  = GL_CALL(glGetUniformLocation(program, "bgra"));

        View = glm::lookAt(glm::vec3(0., 0., 1.67),
                glm::vec3(0., 0., 0.),
                glm::vec3(0., 1., 0.));
        Proj = glm::perspective(45.f, 1.f, .1f, 100.f);

        MVP = glm::mat4();
        GL_CALL(glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]));
    }
}
