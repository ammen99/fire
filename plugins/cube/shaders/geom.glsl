#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec2 tesuv[3];
in vec3 tePosition[3];

uniform mat3 NM;
uniform int  light;

out vec2 guv;
out vec3 colorFactor;

#define AL 0.2    // ambient lighting
#define DL (1-AL) // diffuse lighting
#define DIFFUSE_FACTOR 0.2

float getC(float dist, float maxd) {
    float c01 = abs(dist) / maxd;
    float pw = pow(100, c01) - 1;

    return pw / 100;
}

void main() {

    if(light == 1) {
        const vec3 eye = vec3(0, 2, 2);

        vec3 tmp = (tePosition[0] + tePosition[1] + tePosition[2]) / 3.0;
        float d = distance(tmp, eye);
        float f = getC(d, 0.1);
        d = d * f;

        vec3 A = tePosition[2] - tePosition[0];
        vec3 B = tePosition[1] - tePosition[0];
        vec3 N = normalize(NM * normalize(cross(A, B)));
        vec3 L = vec3(0, 0, 1.5);
        float angle = abs(dot(N, L));

        float df = clamp(angle * DIFFUSE_FACTOR + d, 0, 1);
        colorFactor = vec3(AL, AL, AL) + df * vec3(DL, DL, DL);
    }
    else
        colorFactor = vec3(1, 1, 1);

    gl_Position = gl_in[0].gl_Position;
    guv = tesuv[0];
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    guv = tesuv[1];
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    guv = tesuv[2];
    EmitVertex();
}
