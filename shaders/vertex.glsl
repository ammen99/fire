#version 100

attribute  mediump vec3 position;
attribute  mediump vec2 uvPosition;

varying mediump vec2 uvpos;
mediump vec3 vPos;

uniform mat4 MVP;
uniform float w2;
uniform float h2;

void main() {
    vPos.x = position.x / w2;
    vPos.y = position.y / h2;
    vPos.z = position.z;

    gl_Position = MVP * vec4(vPos, 1.0);
    uvpos = uvPosition;
}
