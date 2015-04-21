#version 440

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec2 tesuv[3];
in vec3 tePosition[3];

out vec2 guv;

out vec3 gFacetNormal;

uniform mat3 NormalMatrix;

void main() {

    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = NormalMatrix * normalize(cross(A, B));

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
