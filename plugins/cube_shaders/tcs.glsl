#version 440

layout(vertices = 3) out;

in vec2 uvpos[];
in vec3 vPos[];

out vec3 tcPosition[];
out vec2 uv[];

#define ID gl_InvocationID

void main() {

    tcPosition[ID] = vPos[ID];
    uv[ID] = uvpos[ID];

    if(ID == 0){
        gl_TessLevelInner[0] = 35;
        gl_TessLevelOuter[0] = 35;
        gl_TessLevelOuter[1] = 35;
        gl_TessLevelOuter[2] = 35;
    }
}
