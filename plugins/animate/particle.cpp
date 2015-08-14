#include "particle.hpp"
#include <opengl.hpp>

glm::vec4 operator * (glm::vec4 v, float x) {
    v[0] *= x;
    v[1] *= x;
    v[2] *= x;
    v[3] *= x;
    return v;
}

glm::vec4 operator / (glm::vec4 v, float x) {
    v[0] /= x;
    v[1] /= x;
    v[2] /= x;
    v[3] /= x;
    return v;
}

/* Implementation of Particle */
Particle::Particle() {}
Particle::~Particle() {}
Particle::Particle(int maxLife,
                glm::vec4 startColor,
                glm::vec4 targetColor,
                float cX, float cY,
                float dirX, float dirY) {

    this->maxLife = maxLife;
    this->startColor  = startColor;
    this->targetColor = targetColor;
    this->centerX = cX;
    this->centerY = cY;
    this->dirX = dirX;
    this->dirY = dirY;

    this->life = 0;
    update();
}

bool Particle::update() {
    currentColor = targetColor * life + startColor * (maxLife - life);
    currentColor = currentColor / float(maxLife);

    centerX += dirX;
    centerY += dirY;

    ++life;
    return life <= maxLife;
}

void Particle::getPosition(float &x, float &y) {x = centerX, y = centerY;}
glm::vec4 Particle::getColor() {return currentColor;}

/* Implementation of ParticleSystem */

void ParticleSystem::initGLPart() {

    /* load particle program */
    program = glCreateProgram();
    GLuint vss, fss;
    std::string shaderSrcPath = "/usr/local/share/fireman/animate/pt";

    vss = GLXUtils::loadShader(std::string(shaderSrcPath)
            .append("/vertex.glsl").c_str(), GL_VERTEX_SHADER);

    fss = GLXUtils::loadShader(std::string(shaderSrcPath)
            .append("/frag.glsl").c_str(), GL_FRAGMENT_SHADER);

    glAttachShader (program, vss);
    glAttachShader (program, fss);

    glBindFragDataLocation (program, 0, "outColor");
    glLinkProgram (program);
    glUseProgram(program);


    /* generate VAO and VBOs */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &pbase);

    glGenBuffers(1, &pcolor);
    glBindBuffer(GL_ARRAY_BUFFER, pcolor);
    glBufferData(GL_ARRAY_BUFFER, 24 * maxParticles * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    glGenBuffers(1, &poffset);
    glBindBuffer(GL_ARRAY_BUFFER, poffset);
    glBufferData(GL_ARRAY_BUFFER, 12 * maxParticles * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    for(int i = 0; i < sizeof(vertices) / sizeof(float); i++)
        vertices[i] *= particleSize;

    /* upload static base mesh */
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, pbase);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
            vertices, GL_STATIC_DRAW);
    glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDisableVertexAttribArray(0);
}

Particle *ParticleSystem::newParticle()  {
        float dx = float(std::rand() % 1001 - 500) / (500 * particleLife);
        float dy = float(std::rand() % 1001 - 500) / (500 * particleLife);

        return new Particle(particleLife, glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1),
                0, 0, dx, dy);
}

ParticleSystem::ParticleSystem(float size,  size_t _maxp,
            size_t _pspawn, size_t _plife, size_t _respInterval) {

    particleSize = size;

    this->maxParticles    = _maxp;
    this->partSpawn       = _pspawn;
    this->particleLife    = _plife;
    this->respawnInterval = _respInterval;

    std::cout << maxParticles << std::endl;
    _particles.resize(maxParticles);
    colorBuffer = new float[24 * maxParticles];
    offsetBuffer = new float[12 * maxParticles];

    /* mark unused particles */
    for(int i = 0; i < maxParticles; i++) {
        _particles[i] = nullptr,
        freePlaces.insert(i);
    }

    initGLPart();
}

ParticleSystem::~ParticleSystem() {
    GLuint buffs[] = { bgVBO, pbase, pcolor, poffset };

    glDeleteBuffers(4, buffs);

    glDeleteVertexArrays(1, &bgVAO);
    glDeleteVertexArrays(1, &vao);

    glDeleteProgram(program);
    glDeleteProgram(backgroundProgram);
}

void ParticleSystem::spawnNew() {

    int particlesToSpawn = std::min(partSpawn, freePlaces.size());
    for(int i = 0; i < particlesToSpawn; i++) {
        auto new_place = *freePlaces.begin();
        _particles[new_place] = newParticle();
        freePlaces.erase(new_place);
    }
}

void ParticleSystem::simulate() {
    for(int i = 0; i < maxParticles; i++) {
        if(_particles[i]) {
            if(!_particles[i]->update()) {
                delete _particles[i];
                _particles[i] = nullptr;
                freePlaces.insert(i);
            }
        }
    }

    if(currentIteration % respawnInterval == 0)
        spawnNew();
    ++currentIteration;
}


/* TODO: use glDrawElementsInstanced instead of glDrawArraysInstanced */
void ParticleSystem::render(GLuint backgroundTex) {

    glUseProgram(program);
    std::cout << "somewhere here " << std::endl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);

    /* prepare vertex data */
    for(int i = 0; i < maxParticles; i++) {
        float cx = -10000, cy = -10000;
        glm::vec4 c(0, 0, 0, 0);

        if(_particles[i]) {
            c = _particles[i]->getColor();
            _particles[i]->getPosition(cx, cy);
        }

        for(int j = 0; j < 6; j++) {
            for(int k = 0; k < 4; k++)
                colorBuffer[i * 24 + j * 4 + k] = c[k];

            offsetBuffer[i * 12 + j * 2 + 0] = cx;
            offsetBuffer[i * 12 + j * 2 + 1] = cy;
        }
    }
    /* upload data to GPU */

    size_t a = maxParticles * 24 * sizeof(GLfloat);
    size_t b = maxParticles * 12 * sizeof(GLfloat);

    glBindBuffer(GL_ARRAY_BUFFER, pcolor);
    glBufferData(GL_ARRAY_BUFFER, a, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, a, &colorBuffer[0]);


    glBindBuffer(GL_ARRAY_BUFFER, poffset);
    glBufferData(GL_ARRAY_BUFFER, b, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, b, &offsetBuffer[0]);

    /* prepare vertex attribs */
    glEnableVertexAttribArray(0);
    glBindBuffer (GL_ARRAY_BUFFER, pbase);
    glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, poffset);
    glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, pcolor);
    glVertexAttribPointer (2, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    /* draw particles */
    glDrawArraysInstanced (GL_TRIANGLES, 0, 6, maxParticles * 6);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    glUseProgram(0);

    std::cout << "ENDED LOOK IN MAIN!!!" << std::endl;
}
