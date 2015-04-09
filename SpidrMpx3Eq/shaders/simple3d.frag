#version 330
uniform sampler3D dataTex;
uniform vec3 range;
void main(void)
{
    float depth =   (2.0 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) /
            (gl_DepthRange.far - gl_DepthRange.near);
    depth /= gl_FragCoord.z;
    float texel  = texture(dataTex, vec3(gl_FragCoord.xy/range.xy, depth)).x;
    gl_FragColor =  vec4(vec3(texel), 1);
    //gl_FragColor = vec4(vec3(depth), 1);
}

