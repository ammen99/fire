#include "../include/core.hpp"
#include "../include/opengl.hpp"

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

    GLuint vpID;
    GLuint initialModel;
    GLuint nmID;

    glm::mat4 vp, model, view;
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

            int val = options["deform"]->data.ival;
            glUseProgram(program);
            GLuint defID = glGetUniformLocation(program, "deform");
            glUniform1i(defID, val);

            val = options["light"]->data.ival ? 1 : 0;
            GLuint lightID = glGetUniformLocation(program, "light");
            glUniform1i(lightID, val);

            OpenGL::useDefaultProgram();
        }

        void init() {
            options.insert(newFloatOption("velocity",  0.01));
            options.insert(newFloatOption("vvelocity", 0.01));
            options.insert(newFloatOption("zvelocity", 0.05));
            options.insert(newIntOption  ("deform",    0));
            options.insert(newIntOption  ("light",     false));

            auto shaderSrcPath =
                "/home/ilex/work/cwork/fire/plugins/cube_shaders/";
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
            glUseProgram(program);

            vpID = glGetUniformLocation(program, "VP");
            initialModel = glGetUniformLocation(program, "initialModel");
            nmID = glGetUniformLocation(program, "NM");
            auto proj = glm::perspective(45.0f, 1.f, 0.1f, 100.f);

            view = glm::lookAt(glm::vec3(0., 2., 2),
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
            zoomOut.action = zoomIn.action =
                std::bind(std::mem_fn(&Cube::onScrollEvent), this, _1);

            core->addBut(&zoomOut, false);
            core->addBut(&zoomIn , false);


            activate.button = Button1;
            activate.type = BindingTypePress;
            activate.mod = ControlMask | Mod1Mask;
            activate.action =
                std::bind(std::mem_fn(&Cube::Initiate), this, _1);
            core->addBut(&activate, true);

            deactiv.button = Button1;
            deactiv.mod    = AnyModifier;
            deactiv.type   = BindingTypeRelease;
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
                return;
            }

            GetTuple(vx, vy, core->getWorkspace());

            /* important: core uses vx = col vy = row */
            this->vx = vx, this->vy = vy;

            GetTuple(mx, my, core->getMouseCoord());
            px = mx, py = my;

            mouse.enable();
            deactiv.enable();
            zoomIn.enable();
            zoomOut.enable();

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

            glm::mat4 addedS = scale * verticalRotation;
            glm::mat4 vpUpload = vp * addedS;
            glUniformMatrix4fv(vpID, 1, GL_FALSE, &vpUpload[0][0]);

            for(int i = 0; i < sides.size(); i++) {
                int index = (vx + i) % sides.size();

                glBindTexture(GL_TEXTURE_2D, sides[index]);

                model = glm::rotate(glm::mat4(),
                        float(i) * angle + offset, glm::vec3(0, 1, 0));
                model = glm::translate(model, glm::vec3(0, 0, coeff));

                auto nm =
                    glm::inverse(glm::transpose(glm::mat3(view *  addedS)));

                glUniformMatrix4fv(initialModel, 1, GL_FALSE, &model[0][0]);
                glUniformMatrix3fv(nmID, 1, GL_FALSE, &nm[0][0]);

                glPatchParameteri(GL_PATCH_VERTICES, 3);
                glDrawArrays (GL_PATCHES, 0, 6);
            }
            glXSwapBuffers(core->d, core->outputwin);
        }

        void Terminate(Context *ctx) {
            OpenGL::useDefaultProgram();
            core->setDefaultRenderer();
            mouse.disable();
            deactiv.disable();
            zoomIn.disable();
            zoomOut.disable();
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
