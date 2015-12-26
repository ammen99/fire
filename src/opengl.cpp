#include <opengl.hpp>
OpenGL::_API OpenGL::API;

namespace {

    namespace {
        void *handle;
    }

#define NAME(x) gl ## x
#define load(x) \
    OpenGL::API. gl ## x = (decltype(OpenGL::API. gl ## x))dlsym(handle, "gl" #x); \
    if(!OpenGL::API.gl ## x) { \
        std::cout << "failed to load " << #x << std::endl; \
        std::exit(-1); \
    }

    void load_gl() {
        handle = dlopen("libGL.so", RTLD_LAZY);

        load(ActiveTexture);
        load(AttachShader);
        load(BindBuffer);
        load(BindFragDataLocation);
        load(BindFramebuffer);
        load(BindVertexArray);
        load(BufferData);
        load(CompileShader);
        load(CreateProgram);
        load(CreateShader);
        load(DeleteBuffers);
        load(DeleteVertexArrays);
        load(EnableVertexAttribArray);
        load(FramebufferTexture);
        load(GenBuffers);
        load(GenFramebuffers);
        load(GenVertexArrays);
        load(GetAttribLocation);
        load(GetShaderInfoLog);
        load(GetShaderiv);
        load(GetUniformLocation);
        load(LinkProgram);
        load(ShaderSource);
        load(Uniform1f);
        load(Uniform1i);
        load(Uniform4fv);
        load(UniformMatrix4fv);
        load(UseProgram);
        load(VertexAttribPointer);
        load(CheckFramebufferStatus);
        load(DeleteTextures);
    };

    GLuint program;
    GLuint mvpID;
    GLuint depthID, colorID, bgraID;
    glm::mat4 View;
    glm::mat4 Proj;
    glm::mat4 MVP;

    GLuint framebuffer;
    GLuint framebufferTexture;

    GLuint fullVAO, fullVBO;

   /*these functions are disabled for now

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
        if(severity == GL_DEBUG_SEVERITY_HIGH_ARB  )return "HIGH";
        if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)return "MEDIUM";
        if(severity == GL_DEBUG_SEVERITY_LOW_ARB   )return "LOW";
        if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) return "NOTIFICATION";
        return "UNKNOWN";
    }

    void errorHandler(GLenum src, GLenum type,
            GLuint id, GLenum severity,
            GLsizei len, const GLchar *msg,
            const void *dummy) {

        // ignore notifications
        if(severity == GL_DEBUG_SEVERITY_NOTIFICATION)
            return;

        std::cout << "_______________________________________________\n";
        std::cout << "OGL debug: \n";
        std::cout << "Source: " << getStrSrc(src) << std::endl;
        std::cout << "Type: " << getStrType(type) << std::endl;
        std::cout << "ID: " << id << std::endl;
        std::cout << "Severity: " << getStrSeverity(severity) << std::endl;
        std::cout << "Msg: " << msg << std::endl;;
        std::cout << "_______________________________________________\n";
    }
    */
}

bool OpenGL::transformed = false;
int  OpenGL::depth;
glm::vec4 OpenGL::color;

namespace OpenGL {
    int VersionMinor, VersionMajor;

    GLuint getTex() {return framebufferTexture;}

void generateVAOVBO(int x, int y, int w, int h,
        GLuint &vao, GLuint &vbo) {

    int sw = 400;
    int sh = 400;
    //GetTuple(sw, sh, core->getScreenSize());


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

    OpenGL::API.glGenVertexArrays(1, &vao);
    OpenGL::API.glBindVertexArray(vao);

    OpenGL::API.glGenBuffers (1, &vbo);
    OpenGL::API.glBindBuffer (GL_ARRAY_BUFFER, vbo);


    OpenGL::API.glBufferData(GL_ARRAY_BUFFER,
            sizeof(vertexData), vertexData,
            GL_STATIC_DRAW);

    GLint position = OpenGL::API.glGetAttribLocation (program, "position");
    GLint uvPosition = OpenGL::API.glGetAttribLocation (program, "uvPosition");

    OpenGL::API.glVertexAttribPointer (position, 3,
            GL_FLOAT, GL_FALSE,
            5 * sizeof (GL_FLOAT), 0);
    OpenGL::API.glVertexAttribPointer (uvPosition, 2,
            GL_FLOAT, GL_FALSE,
            5 * sizeof (GL_FLOAT), (void*)(3 * sizeof (float)));

    OpenGL::API.glEnableVertexAttribArray (position);
    OpenGL::API.glEnableVertexAttribArray (uvPosition);
}

void generateVAOVBO(int w, int h, GLuint &vao, GLuint &vbo) {
    GetTuple(sw, sh, core->getScreenSize());

    int mx = sw / 2;
    int my = sh / 2;

    generateVAOVBO(mx - w / 2, my - h / 2, w, h, vao, vbo);
}


void renderTexture(GLuint tex, GLuint vao, GLuint vbo) {
    OpenGL::API.glBindVertexArray(vao);
    OpenGL::API.glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays (GL_TRIANGLES, 0, 6);
}

void renderTransformedTexture(GLuint tex,
        GLuint vao, GLuint vbo,
        glm::mat4 Model) {
    if(transformed)
        MVP = Proj * View * Model;
    else
        MVP = Model;

    OpenGL::API.glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
    OpenGL::API.glUniform1i(depthID, depth);
    OpenGL::API.glUniform4fv(colorID, 1, &color[0]);

    renderTexture(tex, vao, vbo);
}

void preStage() {

    OpenGL::API.glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    GetTuple(sw, sh, core->getScreenSize());
    if(FireWin::allDamaged) {
        glScissor(0, 0, sw, sh);
        return;
    }

    XRectangle rect;
    XClipBox(core->dmg, &rect);

    int blx = rect.x;
    int bly = sh - (rect.y + rect.height);
    glScissor(blx, bly, rect.width, rect.height);
}

void preStage(GLuint fbuff) {
    OpenGL::API.glBindFramebuffer(GL_FRAMEBUFFER, fbuff);
    GetTuple(sw, sh, core->getScreenSize());
    glScissor(0, 0, sw, sh);
}

void endStage() {
    OpenGL::API.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto tmp = glm::mat4();
    OpenGL::API.glUniformMatrix4fv(mvpID, 1, GL_FALSE, &tmp[0][0]);
    OpenGL::API.glUniform1i(bgraID, 1);

    OpenGL::color = glm::vec4(1, 1, 1, 1);
    OpenGL::API.glUniform4fv(colorID, 1, &color[0]);

    GetTuple(sw, sh, core->getScreenSize());
    glScissor(0, 0, sw, sh);


    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    renderTexture(framebufferTexture, fullVAO, fullVBO);
    glFinish();

//    uint sync;
//    GLXUtils::glXGetVideoSyncSGI_func(&sync);
//    GLXUtils::glXWaitVideoSyncSGI_func(2, (sync + 1) % 2, &sync);

    glXSwapBuffers(core->d, core->outputwin);
    OpenGL::API.glUniform1i(bgraID, 0);
}

void prepareFramebuffer(GLuint &fbuff, GLuint &texture) {
    GetTuple(sw, sh, core->getScreenSize());

    OpenGL::API.glGenFramebuffers(1, &fbuff);
    OpenGL::API.glBindFramebuffer(GL_FRAMEBUFFER, fbuff);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sw, sh,
            0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    OpenGL::API.glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            texture, 0);

    if(OpenGL::API.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cout << "Error in framebuffer !!!" << std::endl;
    }
}

void useDefaultProgram() {
    OpenGL::API.glUseProgram(program);
    glClearColor (.0f, .0f, .0f, 1.f);
    glClearDepth (1.f);

    glEnable     (GL_BLEND);
    glBlendFunc  (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable     (GL_SCISSOR_TEST);
    glDisable    (GL_DEPTH_TEST);
}

void initOpenGL(const char *shaderSrcPath) {

    std::cout << "has come to here" << std::endl;

    std::cout << "init glew" << std::endl;

    ilInit();
    iluInit();
    ilutInit();
    ilutRenderer ( ILUT_OPENGL );

    std::cout << "init ilut" << std::endl;

    //glEnable(GL_DEBUG_OUTPUT);
    //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //glDebugMessageCallback(errorHandler, (void*)0);

    load_gl();
    //GetTuple(sw, sh, core->getScreenSize());
    int sw = 400;
    int sh = 400;
    std::string tmp = shaderSrcPath;

    GLuint vss =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/vertex.glsl").c_str(),
                GL_VERTEX_SHADER);

    GLuint fss =
        GLXUtils::loadShader(std::string(shaderSrcPath)
                .append("/frag.glsl").c_str(),
                GL_FRAGMENT_SHADER);

    program = OpenGL::API.glCreateProgram();

    OpenGL::API.glAttachShader (program, vss);
    OpenGL::API.glAttachShader (program, fss);

    OpenGL::API.glBindFragDataLocation (program, 0, "outColor");
    OpenGL::API.glLinkProgram (program);
    useDefaultProgram();

    mvpID = OpenGL::API.glGetUniformLocation(program, "MVP");
    depthID = OpenGL::API.glGetUniformLocation(program, "depth");
    colorID = OpenGL::API.glGetUniformLocation(program, "color");
    bgraID = OpenGL::API.glGetUniformLocation(program, "bgra");

    View = glm::lookAt(glm::vec3(0., 0., 1.67),
                       glm::vec3(0., 0., 0.),
                       glm::vec3(0., 1., 0.));
    Proj = glm::perspective(45.f, 1.f, .1f, 100.f);

    MVP = glm::mat4();

    OpenGL::API.glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);

    auto w2ID = OpenGL::API.glGetUniformLocation(program, "w2");
    auto h2ID = OpenGL::API.glGetUniformLocation(program, "h2");

    OpenGL::API.glUniform1f(w2ID, sw / 2);
    OpenGL::API.glUniform1f(h2ID, sh / 2);

    generateVAOVBO(0, sh, sw, -sh, fullVAO, fullVBO);

    prepareFramebuffer(framebuffer, framebufferTexture);
}
}
