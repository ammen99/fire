#version 330
in vec2 uv;

uniform sampler2D smp;

out vec4 outColor;
void main() {
    outColor = texture(smp, uv).zyxw;
}
