#version 330
uniform sampler3D dataTex;
uniform vec3 range;
void main(void)
{
    float texel  = texture(dataTex, gl_FragCoord.xyz/range).x;
    gl_FragColor =  vec4(texel,0,gl_FragCoord.x/range.x, 1);
}

