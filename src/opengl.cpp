#include "../include/opengl.hpp"

bool OpenGLWorker::transformed = false;

GLuint OpenGLWorker::program;
GLuint OpenGLWorker::mvpID;
GLuint OpenGLWorker::transformID;
GLuint OpenGLWorker::normalID;
GLuint OpenGLWorker::depthID;
GLuint OpenGLWorker::colorID;

glm::mat4 OpenGLWorker::MVP;
glm::mat4 OpenGLWorker::View;
glm::mat4 OpenGLWorker::Proj;
glm::mat3 OpenGLWorker::NM;

int   OpenGLWorker::depth;
glm::vec4 OpenGLWorker::color;

void OpenGLWorker::generateVAOVBO(int x, int y, int w, int h,
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

void OpenGLWorker::generateVAOVBO(int w, int h, GLuint &vao, GLuint &vbo) {
    GetTuple(sw, sh, core->getScreenSize());

    int mx = sw / 2;
    int my = sh / 2;

    generateVAOVBO(mx - w / 2, my - h / 2, w, h, vao, vbo);
}


void OpenGLWorker::renderTexture(GLuint tex, GLuint vao, GLuint vbo) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindTexture(GL_TEXTURE_2D, tex);

    //glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawArrays (GL_TRIANGLES, 0, 6);
}

void OpenGLWorker::renderTransformedTexture(GLuint tex,
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

    MVP = Proj * View;
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
}

void OpenGLWorker::preStage() {
    GetTuple(sw, sh, core->getScreenSize());

    XRectangle rect;
    XClipBox(core->dmg, &rect);

    int blx = rect.x;
    int bly = sh - (rect.y + rect.height);

    if(!__FireWindow::allDamaged) // do not scissor if damaging everything
        glScissor(blx, bly, rect.width, rect.height);
    else
        glScissor(0, 0, sw, sh);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

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

    cout << "_______________________________________________" << std::endl;
    cout << "OGL debug: " << std::endl;
    cout << "Source: " << getStrSrc(src) << std::endl;
    cout << "Type: " << getStrType(type) << std::endl;
    cout << "ID: " << id << std::endl;
    cout << "Severity: " << getStrSeverity(severity) << std::endl;
    cout << "Msg: " << msg << std::endl;
    cout << "_______________________________________________" << std::endl;
}



void OpenGLWorker::initOpenGL(Core *core, const char *shaderSrcPath) {
//
//    glEnable(GL_DEBUG_OUTPUT);
//    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
//    glDebugMessageCallback(errorHandler, (void*)0);
    //
    GetTuple(sw, sh, core->getScreenSize());

    glClearColor (.0f, .0f, .0f, 1.f);
    glClearDepth (1.f);
//    glEnable     (GL_DEPTH_TEST);
    glEnable     (GL_ALPHA_TEST);
    glEnable     (GL_BLEND);
    glBlendFunc  (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glDepthFunc  (GL_LESS);
    glViewport   (0, 0, sw, sh);
//    glDisable    (GL_CULL_FACE);
    glEnable     (GL_SCISSOR_TEST);
//
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

    glAttachShader (program, vss);
    glAttachShader (program, fss);

    glBindFragDataLocation (program, 0, "outColor");
    glLinkProgram (program);
    glUseProgram (program);

    mvpID = glGetUniformLocation(program, "MVP");
    normalID = glGetUniformLocation(program, "NormalMatrix");
    depthID = glGetUniformLocation(program, "depth");
    colorID = glGetUniformLocation(program, "color");
    color = glm::vec4(1.f, 1.f, 1.f, 1.f);

    View = glm::lookAt(glm::vec3(0., 0., 1.67),
                       glm::vec3(0., 0., 0.),
                       glm::vec3(0., 1., 0.));
    Proj = glm::perspective(45.f, 1.f, .1f, 100.f);

    MVP = glm::mat4();
    NM = glm::inverse(glm::transpose(glm::mat3(View)));

    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix3fv(normalID, 1, GL_FALSE, &NM[0][0]);

    auto w2ID = glGetUniformLocation(program, "w2");
    auto h2ID = glGetUniformLocation(program, "h2");

    glUniform1f(w2ID, sw / 2);
    glUniform1f(h2ID, sh / 2);
}

