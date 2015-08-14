#include <opengl.hpp>
#include "fire.hpp"

#define EFFECT_CYCLES 15
#define BURSTS 10
#define MAX_PARTICLES 25000
#define PARTICLE_SIZE 0.003

/* FireParticle is a normal particle but stays into given region */
class FireParticle : public Particle {
    float w, h;
    float cx, cy;

    /* note color order */
    /* we use BGRA notation */
    public:
    FireParticle(float cx, float cy,  float w, float h, float dirx, float diry) :
        Particle(EFFECT_CYCLES,
                glm::vec4(0.0, 0.55, 1.0, 1),
                glm::vec4(0.0, 0.2, 1.0, 0.01),
                cx, cy, dirx, diry) {

            this->w = w;
            this->h = h;

            this->cx = cx;
            this->cy = cy;
    }

    bool update() {
        Particle::update();

        /* check if we are beyond windows's boundaries */
        if(centerX > cx + w) life = maxLife + 1;
        if(centerX < cx - w) life = maxLife + 1;

        if(centerY > cy + h) life = maxLife + 1;
        if(centerY < cy - h) life = maxLife + 1;


        return life <= maxLife;
    }

};

#define avg(x,y) (((x) + (y))/2.0)
#define clamp(t,x,y) t=(t > (y) ? (y) : (t < (x) ? (x) : t))

class FireParticleSystem : public ParticleSystem {
    float _cx, _cy;
    float _w, _h;

    float speedCoeff;

    public:
        FireParticleSystem(float cx, float cy, float w, float h, int numParticles) :
            ParticleSystem(PARTICLE_SIZE, numParticles, numParticles / BURSTS,
                    EFFECT_CYCLES, EFFECT_CYCLES / BURSTS + 1),
            _cx(cx), _cy(cy), _w(w), _h(h) {

                speedCoeff = 1. / EFFECT_CYCLES * 7000.0 * particleLife;
        }

        Particle *newParticle() {
            float dx = _w * float(std::rand() % 1001 - 500) / speedCoeff;
            float dy = _h * float(std::rand() % 1001 - 500) / speedCoeff;

            return new FireParticle(_cx, _cy, _w, _h, dx, dy);
        }

        bool isRunning() {
            return this->currentIteration < EFFECT_CYCLES;
        }
};

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
        MAX_PARTICLES * 2.0 * w / float(w) * 2.0 * h / float(h);
    if(numParticles > MAX_PARTICLES) numParticles = MAX_PARTICLES;

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
        ps->render(tex);

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
