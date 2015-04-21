#version 440

in vec3 position;
in vec2 uvPosition;

out vec2 uvpos;
out vec3 vPos;

void main() {
    vPos.x = position.x / 683.;
    vPos.y = position.y / 384.;
    vPos.z = position.z;

    gl_Position = vec4(vPos, 1.0);
    uvpos = uvPosition;
};
