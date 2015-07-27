#include "../include/core.hpp"
#include "../include/opengl.hpp"
#define GLSL(x) "#version 440\n" # x

const char *vs = GLSL (
    in vec3 pos;
    in vec2 uvPos;

    out vec2 uvpos;
    uniform mat4 MVP;

    void main() {
       gl_Position = MVP * vec4 (pos, 1.0);
       uvpos = uvPos;
    });

const char *fs = GLSL (
    in vec2 uvpos;
    out vec4 outColor;

    uniform sampler2D smp;

    void main() {
       outColor = vec4(texture(smp, uvpos).zyx, 1);
    });
class Cube : public Plugin {
    ButtonBinding activate;
    ButtonBinding deactiv;
    ButtonBinding zoomIn, zoomOut;

    Hook mouse;
    std::vector<GLuint> sides;
    std::vector<GLuint> sideFBuffs;
    int vx, vy;


    float Velocity = 0.01;
    float VVelocity = 0.01;
    float ZVelocity = 0.05;
    float MaxFactor = 10;

    float angle;      // angle between sides
    float offset;     // horizontal rotation angle
    float offsetVert; // vertical rotation angle
    float zoomFactor = 1.0;

    int px, py;

    RenderHook renderer;

    GLuint program;
    GLuint vao, vbo;
    GLuint mvpID;

    glm::mat4 vp, model;
    float coeff;
    public:
        void initOwnership() {
            owner->name = "cube";
            owner->compatAll = false;
        }

        void updateConfiguration() {
            Velocity  = options["velocity" ]->data.fval;
            VVelocity = options["vvelocity"]->data.fval;
            ZVelocity = options["zvelocity"]->data.fval;
        }

        void init() {
            options.insert(newFloatOption("velocity",  0.01));
            options.insert(newFloatOption("vvelocity", 0.01));
            options.insert(newFloatOption("zvelocity", 0.05));

            auto vso = GLXUtils::compileShader(vs, GL_VERTEX_SHADER);
            auto fso = GLXUtils::compileShader(fs, GL_FRAGMENT_SHADER);

            program = glCreateProgram();
            glAttachShader(program, vso);
            glAttachShader(program, fso);
            glBindFragDataLocation(program, 0, "outColor");
            glLinkProgram(program);
            glUseProgram(program);

            mvpID = glGetUniformLocation(program, "MVP");
            auto proj = glm::perspective(45.0f, 1.f, 0.1f, 100.f);

            auto view = glm::lookAt(glm::vec3(0., 2., 2),
                    glm::vec3(0., 0., 0.),
                    glm::vec3(0., 1., 0.));

            vp = proj * view;
            glGenVertexArrays (1, &vao);
            glBindVertexArray (vao);

            GLfloat vertices[] = {
                -0.5f, -0.5f, 0.f, 0.0f, 0.0f,
                0.5f, -0.5f, 0.f, 1.0f, 0.0f,
                0.5f,  0.5f, 0.f, 1.0f, 1.0f,
                0.5f,  0.5f, 0.f, 1.0f, 1.0f,
                -0.5f,  0.5f, 0.f, 0.0f, 1.0f,
                -0.5f, -0.5f, 0.f, 0.0f, 0.0f,
            };
            glGenBuffers (1, &vbo);
            glBindBuffer (GL_ARRAY_BUFFER, vbo);
            glBufferData (GL_ARRAY_BUFFER, sizeof (vertices),
                    vertices, GL_STATIC_DRAW );

            GLint position = glGetAttribLocation (program, "pos");
            GLint uvPosition = glGetAttribLocation (program, "uvPos");

            glVertexAttribPointer (position, 3,
                    GL_FLOAT, GL_FALSE,
                    5 * sizeof (GL_FLOAT), 0);
            glVertexAttribPointer (uvPosition, 2,
                    GL_FLOAT, GL_FALSE,
                    5 * sizeof (GL_FLOAT), (void*)(3 * sizeof (float)));

            glEnableVertexAttribArray (position);
            glEnableVertexAttribArray (uvPosition);

            OpenGL::useDefaultProgram();

            GetTuple(vw, vh, core->getWorksize());
            vh = 0;

            sides.resize(vw);
            sideFBuffs.resize(vw);

            angle = 2 * M_PI / float(vw);
            coeff = 0.5 / std::tan(angle / 2);

            for(int i = 0; i < vw; i++)
                sides[i] = sideFBuffs[i] = -1,
                OpenGL::prepareFramebuffer(sideFBuffs[i], sides[i]);

            using namespace std::placeholders;

            zoomOut.button = Button4;
            zoomIn.button  = Button5;
            zoomOut.type   = zoomIn.type   = BindingTypePress;
            zoomOut.mod    = zoomIn.mod    = AnyModifier;
            zoomOut.active = zoomIn.active = false;
            zoomOut.action = zoomIn.action =
                std::bind(std::mem_fn(&Cube::onScrollEvent), this, _1);

            core->addBut(&zoomOut, false);
            core->addBut(&zoomIn , false);


            activate.button = Button1;
            activate.type = BindingTypePress;
            activate.mod = ControlMask | Mod1Mask;
            activate.active = true;
            activate.action =
                std::bind(std::mem_fn(&Cube::Initiate), this, _1);
            core->addBut(&activate, true);

            deactiv.button = Button1;
            deactiv.mod    = AnyModifier;
            deactiv.type   = BindingTypeRelease;
            deactiv.active = false;
            deactiv.action =
                std::bind(std::mem_fn(&Cube::Terminate), this, _1);
            core->addBut(&deactiv, false);

            mouse.action = std::bind(std::mem_fn(&Cube::mouseMoved), this);
            core->addHook(&mouse);

            renderer = std::bind(std::mem_fn(&Cube::Render), this);
        }

        void Initiate(Context *ctx) {
            if(!core->activateOwner(owner))
                return;
            owner->grab();

            if(!core->setRenderer(renderer)) {
                owner->ungrab();
                core->deactivateOwner(owner);
            }

            GetTuple(vx, vy, core->getWorkspace());

            /* important: core uses vx = col vy = row */
            this->vx = vx, this->vy = vy;

            GetTuple(mx, my, core->getMouseCoord());
            px = mx, py = my;

            mouse.enable();
            deactiv.active = true;
            zoomIn.active = zoomOut.active = true;

            offset = 0;
            offsetVert = 0;
            zoomFactor = 1;
        }

        void Render() {
            glClearColor(1.f, 0.5f, 0.5f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

            for(int i = 0; i < sides.size(); i++) {
                core->getViewportTexture(std::make_tuple(i, vy),
                        sideFBuffs[i], sides[i]);
            }

            glUseProgram(program);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            glm::mat4 verticalRotation = glm::rotate(glm::mat4(),
                    offsetVert, glm::vec3(1, 0, 0));
            glm::mat4 scale = glm::scale(glm::mat4(),
                    glm::vec3(1. / zoomFactor, 1. / zoomFactor,
                        1. / zoomFactor));

            glm::mat4 added = vp * scale * verticalRotation;

            for(int i = 0; i < sides.size(); i++) {
                int index = (vx + i) % sides.size();

                glBindTexture(GL_TEXTURE_2D, sides[index]);

                model = glm::rotate(glm::mat4(),
                        float(i) * angle + offset, glm::vec3(0, 1, 0));
                model = added * glm::translate(model,
                        glm::vec3(0, 0, coeff));

                glUniformMatrix4fv(mvpID, 1, GL_FALSE, &model[0][0]);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            glXSwapBuffers(core->d, core->outputwin);
        }

        void Terminate(Context *ctx) {
            OpenGL::useDefaultProgram();
            core->setDefaultRenderer();
            mouse.disable();
            deactiv.active = false;
            zoomIn.active = zoomOut.active = false;
            core->deactivateOwner(owner);

            auto size = sides.size();

            float dx = -(offset) / angle;
            int dvx = 0;
            if(dx > -1e-4)
                dvx = std::floor(dx + 0.5);
            else
                dvx = std::floor(dx - 0.5);

            int nvx = (vx + (dvx % size) + size) % size;
            core->switchWorkspace(std::make_tuple(nvx, vy));
        }

        void mouseMoved() {
            GetTuple(mx, my, core->getMouseCoord());
            int xdiff = mx - px;
            int ydiff = my - py;
            offset += xdiff * Velocity;
            offsetVert += ydiff * VVelocity;
            px = mx, py = my;
        }

        void onScrollEvent(Context *ctx) {
            auto xev = ctx->xev;
            if(xev.xbutton.button == zoomIn.button)
                zoomFactor -= ZVelocity;
            if(xev.xbutton.button == zoomOut.button)
                zoomFactor += ZVelocity;

            if(zoomFactor <= ZVelocity)
                zoomFactor = ZVelocity;

            if(zoomFactor > MaxFactor)
                zoomFactor = MaxFactor;
        }
};

extern "C" {
    Plugin *newInstance() {
        return new Cube();
    }

}
