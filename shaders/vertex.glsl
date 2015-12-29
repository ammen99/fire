#version 100

attribute  mediump vec3 position;
attribute  mediump vec2 uvPosition;

varying mediump vec2 uvpos;
mediump vec3 vPos;

uniform mat4 MVP;
uniform float w2;
uniform float h2;

uniform int time;

void main() {

    mat4 ortho = mat4(
            1.0 / w2, 0, 0, 0,
            0, -1.0 / h2, 0, 0,
            0, 0, -1, 0,
            -1, 1, 0, 1);

    vPos.x = position.x / w2;
    vPos.y = position.y / h2;
    vPos.z = position.z;

    gl_Position = vec4(vPos, 1.0);
    //gl_Position = MVP * vec4(, 1.0);
    //gl_Position = ortho * vec4(position, 1.0);
    uvpos = uvPosition;
}
