#version 330
smooth in vec2 c;
flat in int frameID;

uniform isampler2DArray tex;
uniform sampler1D gradient;
uniform vec2 clampRange; //uses floats for now. perhaps better to make an ivec2.


float normalizeSample(float sample){
    float clamped = clamp(sample, clampRange.x, clampRange.y);
    return  (clamped-clampRange.x)/(clampRange.y-clampRange.x);
}

void main(void) {
    int sample = texture(tex,vec3(c,frameID)).r;
    vec3 color = texture(gradient, normalizeSample(float(sample))).rgb;
    gl_FragColor =  vec4(color, 1);
    //gl_FragColor = vec4(1);
}
