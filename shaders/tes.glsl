#version 440
layout(triangles) in;

in vec3 tcPosition[];
in vec2 uv[];

out vec2 tesuv;
out vec3 tePosition;

uniform mat4 MVP;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
    return vec2(gl_TessCoord.x) * v0
         + vec2(gl_TessCoord.y) * v1
         + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return vec3(gl_TessCoord.x) * v0
         + vec3(gl_TessCoord.y) * v1
         + vec3(gl_TessCoord.z) * v2;
}

vec3 tp;
void main() {
    tesuv = interpolate2D(uv[0], uv[1], uv[2]);
    tePosition = interpolate3D(tcPosition[0], tcPosition[1], tcPosition[2]);
    tp = tePosition;
    gl_Position = MVP * vec4 (tp, 1.0);
}
