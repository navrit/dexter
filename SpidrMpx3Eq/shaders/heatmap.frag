#version 330
smooth in vec2 c;
flat in int frameID;

#define s2(a, b)				temp = a; a = min(a, b); b = max(temp, b);
#define mn3(a, b, c)			s2(a, b); s2(a, c);
#define mx3(a, b, c)			s2(b, c); s2(a, c);

#define mnmx3(a, b, c)			mx3(a, b, c); s2(a, b);                                   // 3 exchanges
#define mnmx4(a, b, c, d)		s2(a, b); s2(c, d); s2(a, c); s2(b, d);                   // 4 exchanges
#define mnmx5(a, b, c, d, e)	s2(a, b); s2(c, d); mn3(a, c, e); mx3(b, d, e);           // 6 exchanges
uniform isampler2DArray tex;
uniform sampler1D gradient;
uniform vec2 clampRange; //uses floats for now. perhaps better to make an ivec2.


float normalizeSample(float sample){
    float clamped = clamp(sample, clampRange.x, clampRange.y);
    return  (clamped-clampRange.x)/(clampRange.y-clampRange.x);
}

void main(void) {
    int v[5];
    int temp;
    v[0] = texture(tex,vec3(c+vec2(0,0),frameID)).r;
    v[1] = texture(tex,vec3(c+vec2(1,0),frameID)).r;
    v[2] = texture(tex,vec3(c+vec2(-1,0),frameID)).r;
    v[3] = texture(tex,vec3(c+vec2(0,1),frameID)).r;
    v[4] = texture(tex,vec3(c+vec2(0,-1),frameID)).r;
    mnmx5(v[0], v[1], v[2], v[3], v[4]);
    mnmx3(v[1], v[2], v[3]);
    int sample = v[2];

    vec3 color = texture(gradient, normalizeSample(float(sample))).rgb;
    gl_FragColor =  vec4(color, step(clampRange.x, sample)* step(sample,clampRange.y));
    //gl_FragColor = vec4(1);
}
