#version 440 
layout(triangles) in;

in vec3 tcPosition[];
in vec2 uv[];

out vec2 tesuv;

out vec3 tePosition;

uniform mat4 MVP;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
    return vec2(gl_TessCoord.x) * v0
         + vec2(gl_TessCoord.y) * v1
         + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return vec3(gl_TessCoord.x) * v0
         + vec3(gl_TessCoord.y) * v1
         + vec3(gl_TessCoord.z) * v2;
}


vec3 tp;
void main() {
    float r = sqrt(2 * .7 * .7);

    tesuv = interpolate2D(uv[0], uv[1], uv[2]);
    tePosition = interpolate3D(tcPosition[0], tcPosition[1], tcPosition[2]);

    float d = distance(tePosition, vec3(0.0, 0.0, 0.0));
    float coeff = d / r;
//    if( d > r ) {
//        tp.x = tePosition.x  / coeff;
//        tp.y = tePosition.y / coeff;
//        tp.z = tePosition.z / coeff;
//    }
//    else
    tp = tePosition;

    //tp.x = clamp(tp.x, -1.0, 1.0);
    //tp.y = clamp(tp.y, -.7, .7);
    //tp.z = clamp(tp.x, -1.0, 1.0);

    gl_Position = MVP * vec4 (tp, 1.0);
    //gl_Position = vec4(tp, 1.0);
}
