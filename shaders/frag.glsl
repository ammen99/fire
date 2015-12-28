#version 100

varying mediump vec2 uvpos;


uniform sampler2D smp;
uniform int       depth;
uniform mediump vec4      color;
uniform int       bgra;

//out mediump vec4 gl_FragColor;


void main() {

    //if(depth == 32)
        //gl_FragColor = texture(smp, uvpos) * color;
    //else
        gl_FragColor = vec4(texture2D(smp, uvpos).xyz, 1);

    //if(bgra == 1)
    //    gl_FragData[0] = gl_FragData[0].zyxw;
}
