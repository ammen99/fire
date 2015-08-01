#version 440

in vec2 guv;
layout(location = 0) out vec4 outColor;

uniform sampler2D smp;

void main() {
    outColor = vec4(texture(smp, guv).zyx, 1);
}
