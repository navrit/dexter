#version 330
uniform sampler2D dataTexture;
in vec2 texCoords;

void main(void)
{
    gl_FragColor = texture2D(dataTexture, texCoords);
}

