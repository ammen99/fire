#version 330

in vec2 uvpos;
layout(location = 0) out vec4 outColor;


uniform sampler2D smp;
uniform int       depth;
uniform vec4      color;
uniform int       bgra;

void main() {
    if(depth == 32)
        outColor = texture(smp, uvpos) * color;
    else
        outColor = vec4(texture(smp, uvpos).xyz, 1) * color;

    if(bgra == 1)
        outColor = outColor.zyxw;
}
