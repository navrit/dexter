#version 330
layout(location = 11) in vec3 in_position;
uniform mat4 mvp_matrix;

void main(void)
{
    gl_Position = mvp_matrix*vec4(in_position,1);
}

