#version 330

in vec3 position;
in vec2 uvPosition;

out vec2 uvpos;
out vec3 vPos;

uniform mat4 MVP;

void main() {
    vPos.x = position.x / 683.;
    vPos.y = position.y / 384.;
    vPos.z = position.z;

    gl_Position = MVP * vec4(vPos, 1.0);
    uvpos = uvPosition;
};
