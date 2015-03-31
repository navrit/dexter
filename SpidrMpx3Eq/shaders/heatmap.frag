#version 330
varying vec2 texCoords;

uniform vec2 resolution;
uniform isampler2DArray tex;
uniform int layer;

uniform sampler1D gradient;
uniform vec2 clampRange; //uses floats for now. perhaps better to make an ivec2.


float normalizeSample(float sample){
    float clamped = clamp(sample, clampRange.x, clampRange.y);
    return  (clamped-clampRange.x)/(clampRange.y-clampRange.x);
}

void main(void) {
    int sample = texture(tex,vec3(texCoords,layer)).r;
    vec3 color = texture(gradient, normalizeSample(float(sample))).rgb;
    gl_FragColor =  vec4(color,1);
    //gl_FragColor = vec4(texCoords, 1, 1);
}
