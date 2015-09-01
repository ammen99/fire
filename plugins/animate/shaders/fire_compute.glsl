#version 440
#extension GL_ARB_compute_variable_group_size : enable

#define PARTICLE_DEAD 0
#define PARTICLE_RESP  1
#define PARTICLE_ALIVE 2
#define PARTICLE_AFTER_LIVING 3

layout(location = 1) uniform float maxLife;

layout(location = 2) uniform vec4 scol;
layout(location = 3) uniform vec4 ecol;

layout(location = 4) uniform float maxw;
layout(location = 5) uniform float maxh;

/* used only for Upwards movement */
layout(location = 6) uniform float wind;

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

#define EC 1.05
#define EEC 1.2

void main() {
    uint i = gl_GlobalInvocationID.x;
    Particle p = _particles[i];

    if(abs(maxLife - p.life) <= 1e-5) {
        lifeInfo[i] = PARTICLE_AFTER_LIVING;
    }

    if(p.life > maxLife) {
        if(lifeInfo[i] == PARTICLE_RESP) {

            //            p.x = p.y = 0;
            p.life = 0;

            p.r = scol.x;
            p.g = scol.y;
            p.b = scol.z;
            p.a = scol.w;

            lifeInfo[i] = PARTICLE_ALIVE;
        }
        else if(lifeInfo[i] == PARTICLE_AFTER_LIVING) {
            p.x = -100;
            p.y = -100;
            _particles[i] = p;
            lifeInfo[i] = PARTICLE_AFTER_LIVING;
            return;
        }
    }

    if(lifeInfo[i] == PARTICLE_ALIVE ||
       lifeInfo[i] == PARTICLE_AFTER_LIVING) {

        vec4 tmp = (ecol - scol) / maxLife;

        p.life += 1;

        p.x += p.dx * maxw;
        p.y += p.dy * maxh + wind;

        float r1 = (rand(vec2(p.x, p.y)) - 0.5) * abs(maxw) / 40.;
        float r2 = (rand(vec2(p.y, p.x)) - 0.5) * abs(maxh) / 20.;

        p.x += r1;
        p.y += r2;


        //
        //    p.x = clamp(p.x, -maxw * (EC - 1), 2 * maxw * EC);

        //    if(p.y > 2 * maxh * EC) {
        //        float r = rand(vec2(p.x, p.y));
        //        if(r > 0.5) {
        //            p.y = clamp(p.y, -2, 2 * maxh * EEC);
        //        }
        //        else {
        //            p.y = clamp(p.y, -2, 2 * maxh * EC);
        //        }
        //    }
        //
        p.r += tmp.x;
        p.g += tmp.y;
        p.b += tmp.z;
        p.a += tmp.w;
    }

    _particles[i] = p;
}
