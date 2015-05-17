#version 440

in vec2 guv;
in vec3 gFacetNormal;
//in vec3 gTriDistance;
//in vec3 gPatchDistance;
//in float gPrimitive;
//in vec2 uvpos;
//in vec3 vPos;

out vec4 outColor;


uniform sampler2D smp;
uniform float     opacity;
uniform int       depth;
uniform vec4      color;

float amplify(float d, float scale, float offset) {
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}
void main() {
//    vec3 N = normalize(gFacetNormal);
//    vec3 L = vec3(.0, .0, 1.0);
//
//    float df = abs(dot(N, L));
//    vec3 color = vec3(0.2, 0.2, 0.2) + df * vec3(0.7, 0.7, 0.7);
//
    //outColor = texture (smp, guv) * vec4(color, 1.0);
    if(depth == 32)
        outColor = texture(smp, guv) * color;
    else
        outColor = vec4(texture(smp, guv).xyz, opacity) * color;
    //outColor = texture(smp, uvpos);
}
