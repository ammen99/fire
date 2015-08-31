#version 440
#extension GL_ARB_compute_variable_group_size : enable

#define PARTICLE_DEAD  0
#define PARTICLE_RESP  1
#define PARTICLE_ALIVE 2

layout(location = 1) uniform float maxLife;

layout(location = 2) uniform vec4 scol;
layout(location = 3) uniform vec4 ecol;

layout(location = 4) uniform float maxw;
layout(location = 5) uniform float maxh;

struct Particle {
    float life;
    float x, y;
    float dx, dy;
    float r, g, b, a;
};

layout(binding = 1) buffer Particles {
    Particle _particles[];
};

layout(binding = 2) buffer ParticleLifeInfo {
    uint lifeInfo[];
};


layout(local_size_variable) in;

float rand(vec2 co){
    return fract(sin(dot(co.xy * 143.729 ,vec2(12.9898,78.233))) * 43758.5453);
}

#define EC 1.2

void main() {
    uint i = gl_GlobalInvocationID.x;
    Particle p = _particles[i];

    if(p.life >= maxLife) {
        if(lifeInfo[i] == PARTICLE_RESP) {

            p.x = p.y = 0;
            p.life = 0;

            p.r = scol.x;
            p.g = scol.y;
            p.b = scol.z;
            p.a = scol.w;

            lifeInfo[i] == PARTICLE_ALIVE;
        }
        else {
            lifeInfo[i] = PARTICLE_DEAD;
            return;
        }
    }

    vec4 tmp = (ecol - scol) / maxLife;

    p.life += 1;

    p.x += p.dx * maxw;
    p.y += p.dy * maxh;

    float r1 = (rand(vec2(p.x, p.y)) - 0.5) * abs(maxw) / 40.;
    float r2 = (rand(vec2(p.y, p.x)) - 0.5) * abs(maxh) / 40.;

    p.x += r1;
    p.y += r2;

    p.x = clamp(p.x, -maxw * EC, maxw * EC);
    p.y = clamp(p.y, -maxh * EC, maxh * EC);

    p.r += tmp.x;
    p.g += tmp.y;
    p.b += tmp.z;
    p.a += tmp.w;

    _particles[i] = p;
}
