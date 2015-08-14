#ifndef PARTICLE_H_
#define PARTICLE_H_

#include <core.hpp>

glm::vec4 operator * (glm::vec4 v, float num);
glm::vec4 operator / (glm::vec4 v, float num);

/* base class for particles */
class Particle {
    protected:

    int maxLife;

    int life;
    float centerX, centerY;

    float dirX, dirY;

    glm::vec4 startColor, targetColor, currentColor;
    public:
        Particle();
        virtual ~Particle();
        Particle(int maxLife,
                glm::vec4 startColor,
                glm::vec4 targetColor,
                float cX = 0.f,
                float cY = 0.f,
                float dirX = 0.f,
                float dirY = 0.f);

        virtual bool  update();
        virtual void  getPosition(float &x, float &y);
        virtual glm::vec4 getColor();
};

/* Config for ParticleSystem */

/* base class for a Particle System
 * system consists of maxParticle particles,
 * initial number is startParticles,
 * each iteration partSpawn particles are spawned */

class ParticleSystem {
    protected:

    std::vector<Particle*> _particles;
    /* where can we put new particles */
    std::unordered_set<size_t> freePlaces;

    /* we spawn particles at each 20th iteration */
    size_t currentIteration = 0;

    size_t maxParticles;
    size_t startParticles;
    size_t partSpawn;
    size_t particleLife;
    size_t respawnInterval;

    float particleSize;

    GLint backgroundProgram;
    GLuint bgVAO, bgVBO;

    GLint program;
    GLuint vao, pbase, pcolor, poffset;

    float uv[24] = {
        -1.f, -1.f, 0.0f, 0.0f,
         1.f, -1.f, 1.0f, 0.0f,
         1.f,  1.f, 1.0f, 1.0f,
         1.f,  1.f, 1.0f, 1.0f,
        -1.f,  1.f, 0.0f, 1.0f,
        -1.f, -1.f, 0.0f, 0.0f,
    };

    float vertices[12] = {
        -1.f, -1.f,
         1.f, -1.f,
         1.f,  1.f,
         1.f,  1.f,
        -1.f,  1.f,
        -1.f, -1.f,
    };

    float *colorBuffer, *offsetBuffer;

    /* creates program, VAO, VBO ... */
    virtual void initGLPart();

    virtual Particle *newParticle();
    /* add new particles */
    virtual void spawnNew();

    public:
    ParticleSystem(float particleSize,
            size_t _maxParticles = 5000,
            size_t _numberSpawned = 200,
            size_t _particleLife = 50,
            size_t _respawnInterval = 25);

    virtual ~ParticleSystem();


    /* simulate movement */
    virtual void simulate();

    /* render to screen */
    virtual void render(GLuint backgroundTex = -1);
};

#endif
