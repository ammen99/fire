#include "../include/opengl.hpp"

namespace {
    GLuint program;
    GLuint mvpID;
    GLuint depthID, colorID, bgraID;
    glm::mat4 View;
    glm::mat4 Proj;
    glm::mat4 MVP;

    GLuint framebuffer;
    GLuint framebufferTexture;
    GLuint depthbuffer;

    GLuint fullVAO, fullVBO;

/* these functions are disabled for now
#define cout std::cout

    const char *getStrSrc(GLenum src) {
        if(src == GL_DEBUG_SOURCE_API_ARB            )return "API_ARB        ";
        if(src == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB  )return "WINDOW_SYSTEM  ";
        if(src == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)return "SHADER_COMPILER";
        if(src == GL_DEBUG_SOURCE_THIRD_PARTY_ARB    )return "THIRD_PARTYB   ";
        if(src == GL_DEBUG_SOURCE_APPLICATION_ARB    )return "APPLICATIONB   ";
        if(src == GL_DEBUG_SOURCE_OTHER_ARB          )return "OTHER_ARB      ";
        else return "UNKNOWN";
    }

    const char *getStrType(GLenum type) {
        if(type==GL_DEBUG_TYPE_ERROR_ARB              )return "ERROR_ARB          ";
        if(type==GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)return "DEPRECATED_BEHAVIOR";
        if(type==GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB )return "UNDEFINED_BEHAVIOR ";
        if(type==GL_DEBUG_TYPE_PORTABILITY_ARB        )return "PORTABILITY_ARB    ";
        if(type==GL_DEBUG_TYPE_PERFORMANCE_ARB        )return "PERFORMANCE_ARB    ";
        if(type==GL_DEBUG_TYPE_OTHER_ARB              )return "OTHER_ARB          ";
        return "UNKNOWN";
    }

    const char *getStrSeverity(GLenum severity) {
        if( severity == GL_DEBUG_SEVERITY_HIGH_ARB  )return "HIGH";
        if( severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)return "MEDIUM";
        if( severity == GL_DEBUG_SEVERITY_LOW_ARB   )return "LOW";
        if( severity == GL_DEBUG_SEVERITY_NOTIFICATION) return "NOTIFICATION";
        return "UNKNOWN";
    }

    void errorHandler(GLenum src, GLenum type,
            GLuint id, GLenum severity,
            GLsizei len, const GLchar *msg,
            const void *dummy) {

        cout << "_______________________________________________";
        cout << "OGL debug: ";
        cout << "Source: " << getStrSrc(src);
        cout << "Type: " << getStrType(type);
        cout << "ID: " << id;
        cout << "Severity: " << getStrSeverity(severity);
        cout << "Msg: " << msg;
        cout << "_______________________________________________";
    }
    */
}

bool OpenGL::transformed = false;
int  OpenGL::depth;
glm::vec4 OpenGL::color;

namespace OpenGL {
void generateVAOVBO(int x, int y, int w, int h,
        GLuint &vao, GLuint &vbo) {

    GetTuple(sw, sh, core->getScreenSize());


    float w2 = float(sw) / 2.;
    float h2 = float(sh) / 2.;

    float tlx = float(x) - w2,
          tly = h2 - float(y);

    GLfloat vertexData[] = {
        tlx    , tly - h, 0.f, 0.0f, 1.0f, // 1
        tlx + w, tly - h, 0.f, 1.0f, 1.0f, // 2
        tlx + w, tly    , 0.f, 1.0f, 0.0f, // 3
        tlx + w, tly    , 0.f, 1.0f, 0.0f, // 3
        tlx    , tly    , 0.f, 0.0f, 0.0f, // 4
        tlx    , tly - h, 0.f, 0.0f, 1.0f, // 1
    };

    vao = 0;
    vbo = 0;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers (1, &vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);


    glBufferData(GL_ARRAY_BUFFER,
            sizeof(vertexData), vertexData,
            GL_STATIC_DRAW);

    GLint position = glGetAttribLocation (program, "position");
    GLint uvPosition = glGetAttribLocation (program, "uvPosition");

    glVertexAttribPointer (position, 3,
            GL_FLOAT, GL_FALSE,
            5 * sizeof (GL_FLOAT), 0);
    glVertexAttribPointer (uvPosition, 2,
            GL_FLOAT, GL_FALSE,
            5 * sizeof (GL_FLOAT), (void*)(3 * sizeof (float)));

    glEnableVertexAttribArray (position);
    glEnableVertexAttribArray (uvPosition);
}

void generateVAOVBO(int w, int h, GLuint &vao, GLuint &vbo) {
    GetTuple(sw, sh, core->getScreenSize());

    int mx = sw / 2;
    int my = sh / 2;

    generateVAOVBO(mx - w / 2, my - h / 2, w, h, vao, vbo);
}


void renderTexture(GLuint tex, GLuint vao, GLuint vbo) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawArrays (GL_PATCHES, 0, 6);
}

void renderTransformedTexture(GLuint tex,
        GLuint vao, GLuint vbo,
        glm::mat4 Model) {
    if(transformed)
        MVP = Proj * View * Model;
    else
        MVP = Model;

    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
    glUniform1i(depthID, depth);
    glUniform4fv(colorID, 1, &color[0]);

    renderTexture(tex, vao, vbo);
}

void preStage() {

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GetTuple(sw, sh, core->getScreenSize());

    XRectangle rect;
    XClipBox(core->dmg, &rect);

    int blx = rect.x;
    int bly = sh - (rect.y + rect.height);

    if(!__FireWindow::allDamaged) // do not scissor if damaging everything
        glScissor(blx, bly, rect.width, rect.height);
    else
        glScissor(0, 0, sw, sh);
}

void endStage() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto tmp = glm::mat4();
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &tmp[0][0]);
    glUniform1i(bgraID, 1);

    GetTuple(sw, sh, core->getScreenSize());
    glScissor(0, 0, sw, sh);

    renderTexture(framebufferTexture, fullVAO, fullVBO);
    glXSwapBuffers(core->d, core->outputwin);

    glUniform1i(bgraID, 0);
}

void initOpenGL(const char *shaderSrcPath) {

    //glEnable(GL_DEBUG_OUTPUT);
    //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //glDebugMessageCallback(errorHandler, (void*)0);

    GetTuple(sw, sh, core->getScreenSize());

    glClearColor (.0f, .0f, .0f, 1.f);
    glClearDepth (1.f);
//    glEnable     (GL_DEPTH_TEST);
    glEnable     (GL_ALPHA_TEST);
    glEnable     (GL_BLEND);
    glBlendFunc  (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glDepthFunc  (GL_LESS);
    glViewport   (0, 0, sw, sh);
    glDisable    (GL_CULL_FACE);
    glEnable     (GL_SCISSOR_TEST);

    std::string tmp = shaderSrcPath;

    GLuint vss =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/vertex.glsl").c_str(),
                GL_VERTEX_SHADER);

    GLuint fss =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/frag.glsl").c_str(),
                GL_FRAGMENT_SHADER);

    GLuint tcs =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/tcs.glsl").c_str(),
                GL_TESS_CONTROL_SHADER);

    GLuint tes =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/tes.glsl").c_str(),
                GL_TESS_EVALUATION_SHADER);

    GLuint gss =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/geom.glsl").c_str(),
                GL_GEOMETRY_SHADER);


    program = glCreateProgram();

    glAttachShader (program, vss);
    glAttachShader (program, fss);
    glAttachShader (program, tcs);
    glAttachShader (program, tes);
    glAttachShader (program, gss);
    glBindFragDataLocation (program, 0, "outColor");
    glLinkProgram (program);
    glUseProgram (program);

    mvpID = glGetUniformLocation(program, "MVP");
    depthID = glGetUniformLocation(program, "depth");
    colorID = glGetUniformLocation(program, "color");
    bgraID = glGetUniformLocation(program, "bgra");
    color = glm::vec4(1.f, 1.f, 1.f, 1.f);

    View = glm::lookAt(glm::vec3(0., 0., 1.67),
                       glm::vec3(0., 0., 0.),
                       glm::vec3(0., 1., 0.));
    Proj = glm::perspective(45.f, 1.f, .1f, 100.f);

    MVP = glm::mat4();

    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);

    auto w2ID = glGetUniformLocation(program, "w2");
    auto h2ID = glGetUniformLocation(program, "h2");

    glUniform1f(w2ID, sw / 2);
    glUniform1f(h2ID, sh / 2);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);


    glGenRenderbuffers(1, &depthbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, sw, sh);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, depthbuffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebufferTexture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cout << "Error in framebuffer !!!" << std::endl;
    }

    generateVAOVBO(0, sh, sw, -sh, fullVAO, fullVBO);

   // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
}
