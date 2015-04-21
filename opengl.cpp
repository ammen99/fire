#include "opengl.hpp"

bool OpenGLWorker::transformed = false;

GLuint OpenGLWorker::program;
GLuint OpenGLWorker::mvpID;
GLuint OpenGLWorker::transformID;
GLuint OpenGLWorker::normalID;

glm::mat4 OpenGLWorker::MVP;
glm::mat4 OpenGLWorker::View;
glm::mat4 OpenGLWorker::Proj;
glm::mat3 OpenGLWorker::NM;

//using std::cout;
//using std::endl;
#define cout err

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

void OpenGLWorker::generateVAOVBO(int x, int y, int w, int h,
        GLuint &vao, GLuint &vbo) {


    err << "regen vbo ";

    float w2 = float(core->width) / 2.;
    float h2 = float(core->height) / 2.;

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

    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindVertexArray(0);

}


void OpenGLWorker::renderTexture(GLuint tex, GLuint vao, GLuint vbo) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawArrays (GL_PATCHES, 0, 6);
}

void OpenGLWorker::renderTransformedTexture(GLuint tex,
        GLuint vao, GLuint vbo,
        glm::mat4 Model) {
    if(transformed)
        MVP = Proj * View * Model;
    else
        MVP = Model;

    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
    renderTexture(tex, vao, vbo);

    MVP = Proj * View;
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);

}


void OpenGLWorker::preStage() {
    //glClearColor(0.5, 1.0, 1.0, 1.0);
    glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

GLuint dummyVAO, dummyVBO;

void OpenGLWorker::initOpenGL() {
    glClearColor ( .0f, .0f, .0f, 0.0f );
    glClearDepth ( 1.f );
    glEnable ( GL_DEPTH_TEST );
    glEnable ( GL_ALPHA_TEST );
    glEnable ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthFunc ( GL_LESS );
    glViewport ( 0, 0, core->width, core->height );
    glDisable ( GL_CULL_FACE );

    err << glewGetErrorString(glGetError());

    GLuint vss =
        GLXWorker::loadShader("/static/shaders/vertex.glsl",
                GL_VERTEX_SHADER);

    GLuint fss =
        GLXWorker::loadShader("/static/shaders/frag.glsl",
            GL_FRAGMENT_SHADER);

    GLuint tcs =
        GLXWorker::loadShader("/static/shaders/tcs.glsl",
                GL_TESS_CONTROL_SHADER);
    GLuint tes =
        GLXWorker::loadShader("/static/shaders/tes.glsl",
                GL_TESS_EVALUATION_SHADER);
    GLuint gss =
        GLXWorker::loadShader("/static/shaders/geom.glsl",
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

    
    glGenVertexArrays(1, &dummyVAO);
    glBindVertexArray(dummyVAO);
    generateVAOVBO(0, 0, core->width, core->height, dummyVAO, dummyVBO);


    mvpID = glGetUniformLocation(program, "MVP");
    normalID = glGetUniformLocation(program, "NormalMatrix");

    err << std::tan(M_PI/8);
    View = glm::lookAt(glm::vec3(0., 1., 1 ),
                       glm::vec3(0., 0., 0.),
                       glm::vec3(0., 1., 0.));
    Proj = glm::perspective(45.f, 1.f, .1f, 100.f);
    MVP = Proj * View;
    //MVP = glm::mat4();
    NM = glm::inverse(glm::transpose(glm::mat3(View)));
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix3fv(normalID, 1, GL_FALSE, &NM[0][0]);

}

