#include <opengl.hpp>
#include <cmath>
#include "fire.hpp"

#define EFFECT_CYCLES 16
#define BURSTS 10
#define MAX_PARTICLES 1023 * 384
#define PARTICLE_SIZE 0.006
#define MAX_LIFE 32
#define RESP_INTERVAL 1

bool run = true;

#define avg(x,y) (((x) + (y))/2.0)
#define clamp(t,x,y) t=(t > (y) ? (y) : (t < (x) ? (x) : t))

class FireParticleSystem : public ParticleSystem {
    float _cx, _cy;
    float _w, _h;

    float wind, gravity;

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
        glUniform1f(5, _w);
        glUniform1f(6, _h);
    }

    void genBaseMesh() {
        ParticleSystem::genBaseMesh();
        for(int i = 0; i < 6; i++)
            vertices[2 * i + 0] += _cx - _w,
            vertices[2 * i + 1] += _cy - _h;
    }

    void defaultParticleIniter(Particle &p) {
        p.life = particleLife + 1;

        p.dy = 2. * float(std::rand() % 1001) / (1000 * EFFECT_CYCLES);
        p.dx = 0;

        p.x = (float(std::rand() % 1001) / 1000.0) * _w * 2.;
        p.y = 0;

        p.r = p.g = p.b = p.a = 0;
    }

    FireParticleSystem(float cx, float cy,
            float w, float h,
            int numParticles) :
        _cx(cx), _cy(cy), _w(w), _h(h) {

            particleSize = PARTICLE_SIZE;

            gravity = -_h * 0.001;
            wind = -gravity * RESP_INTERVAL * BURSTS * 2;

            maxParticles    = numParticles;
            partSpawn       = numParticles / BURSTS;
            particleLife    = MAX_LIFE;
            respawnInterval = RESP_INTERVAL;

            initGLPart();
            setParticleColor(glm::vec4(0, 0.5, 1, 1), glm::vec4(0, 0, 0.7, 0.2));

            glUseProgram(renderProg);
            glUniform1f(1, particleSize);
            glUseProgram(0);

            //wind = 0;
        }

    /* checks if PS should run further
     * and if we should stop spawning */
    int numSpawnedBursts = 0;
    bool check() {

        if((currentIteration - 1) % respawnInterval == 0)
            ++numSpawnedBursts;

        if(numSpawnedBursts >= BURSTS)
            pause();

        return currentIteration <= EFFECT_CYCLES;
    }

    void simulate() {
        glUseProgram(computeProg);
        glUniform1f(7, wind);
        ParticleSystem::simulate();

        wind += gravity;
    }
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
        MAX_PARTICLES *  w / float(sw) * h / float(sh);
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

    if(w->isVisible()) {
        glFinish();
        ps->render();
    }
//
    if(!ps->check()) {
        w->transform.color[3] = 1;

        core->remHook(transparency.id);
        core->remEffect(hook.id);
        delete this;
    }
}

void Fire::adjustAlpha() {

    float c = float(progress) / float(EFFECT_CYCLES);
    c = std::pow(100, c) / 100.;

    w->transform.color[3] = c;
    ++progress;
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
