#include <opengl.hpp>
#include "fire.hpp"

#define EFFECT_CYCLES 128
#define BURSTS 8
#define MAX_PARTICLES 256 * 384
#define PARTICLE_SIZE 0.003

bool run = true;

#define avg(x,y) (((x) + (y))/2.0)
#define clamp(t,x,y) t=(t > (y) ? (y) : (t < (x) ? (x) : t))

class FireParticleSystem : public ParticleSystem {
    float _cx, _cy;
    float _w, _h;

    public:

    void loadComputeProgram() {
        std::string shaderSrcPath = "/usr/local/share/fireman/animate/shaders";

        computeProg = glCreateProgram();
        GLuint css =
            GLXUtils::loadShader(std::string(shaderSrcPath)
                    .append("/fire_compute.glsl").c_str(),
                    GL_COMPUTE_SHADER);

        glAttachShader(computeProg, css);
        glLinkProgram(computeProg);
        glUseProgram(computeProg);

        glUniform1f(1, particleLife);
        glUniform1f(4, _w);
        glUniform1f(5, _h);
    }

    void genBaseMesh() {
        ParticleSystem::genBaseMesh();
        for(int i = 0; i < 6; i++)
            vertices[2 * i + 0] += _cx,
            vertices[2 * i + 1] += _cy;
    }

    FireParticleSystem(float cx, float cy,
                float w, float h,
                int numParticles) :
        _cx(cx), _cy(cy), _w(w), _h(h) {

            particleSize = PARTICLE_SIZE;

            maxParticles    = numParticles;
            partSpawn       = numParticles / (BURSTS - 1);
            particleLife    = EFFECT_CYCLES;
            respawnInterval = EFFECT_CYCLES / (BURSTS + 1);

            initGLPart();
            setParticleColor(glm::vec4(0, 0.5, 1, 1), glm::vec4(0, 0, 0.7, 0.8));
        }

    bool isRunning() {return this->currentIteration <= EFFECT_CYCLES;}
};
bool first_time = true;

Fire::Fire(FireWindow win) : w(win) {
    auto x = win->attrib.x,
         y = win->attrib.y,
         w = win->attrib.width,
         h = win->attrib.height;

    GetTuple(sw, sh, core->getScreenSize());

    float w2 = float(sw) / 2.,
          h2 = float(sh) / 2.;

    float tlx = float(x) / w2 - 1.,
          tly = 1. - float(y) / h2;

    float brx = tlx + w / w2,
          bry = tly - h / h2;


    int numParticles =
        MAX_PARTICLES *  w / float(w) * h / float(h);
    //if(numParticles > MAX_PARTICLES) numParticles = MAX_PARTICLES;

//    numParticles = MAX_PARTICLES;;

    ps = new FireParticleSystem(avg(tlx, brx), avg(tly, bry),
            w / float(sw), h / float(sh), numParticles);

    hook.action = std::bind(std::mem_fn(&Fire::step), this);
    core->addEffect(&hook);
    hook.enable();

    win->transform.color[3] = 0;
    transparency.action = std::bind(std::mem_fn(&Fire::adjustAlpha), this);
    core->addHook(&transparency);
    transparency.enable();

    step();

    /* TODO : Check if necessary */
    core->setRedrawEverything(true);
    OpenGL::useDefaultProgram();
}

void Fire::step() {
    ps->simulate();

    if(w->isVisible())
        ps->render();
//
    if(!ps->isRunning()) {
        w->transform.color[3] = 1;

        core->remHook(transparency.id);
        core->remEffect(hook.id);
        delete this;
    }
}

void Fire::adjustAlpha() {
    w->transform.color[3] += 1. / float(EFFECT_CYCLES);
}

struct RedrawOnceMoreHook {
    Hook h;
    RedrawOnceMoreHook() {
        h.action = std::bind(std::mem_fn(&RedrawOnceMoreHook::step), this);
        core->addHook(&h);
        h.enable();
    }

    void step() {
        core->damageRegion(core->getMaximisedRegion());
        core->remHook(h.id);
        delete this;
    }
};

Fire::~Fire() {
    delete ps;
    core->setRedrawEverything(false);
    core->damageRegion(core->getMaximisedRegion());

    new RedrawOnceMoreHook();
}
