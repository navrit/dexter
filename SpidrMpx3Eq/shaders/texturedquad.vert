#version 330
in vec4 texturedVertices;
out vec2 texCoords;

void main(void)
{
    gl_Position = vec4(texturedVertices.xy, 0, 1);
    texCoords = texturedVertices.zw;
}

