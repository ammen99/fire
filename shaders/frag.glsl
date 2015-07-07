#version 440

in vec2 guv;
in vec3 gFacetNormal;
out vec4 outColor;


uniform sampler2D smp;
uniform int       depth;
uniform vec4      color;

void main() {
    if(depth == 32)
        outColor = texture(smp, guv) * color;
    else
        outColor = vec4(texture(smp, guv).xyz, 1) * color;
}
