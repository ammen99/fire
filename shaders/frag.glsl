#version 330

in vec2 uvpos;
out vec4 outColor;


uniform sampler2D smp;
uniform int       depth;
uniform vec4      color;

void main() {
    if(depth == 32)
        outColor = texture(smp, uvpos).zyxw * color;
    else
        outColor = vec4(texture(smp, uvpos).zyx, 1) * color;
}
