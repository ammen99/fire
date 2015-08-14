#version 330

in vec4 out_color;
out vec4 outColor;

void main() {
    outColor = out_color;
   // outColor = vec4(out_color.xyz, 1.0);
   // outColor = vec4(1, 1, 1, 1);
}
