#version 330

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uvpos;

out vec2 uv;

void main() {
    //position = position * (-1);
    gl_Position = vec4 (position, 0.0, 1.0);
    uv = uvpos;
}


