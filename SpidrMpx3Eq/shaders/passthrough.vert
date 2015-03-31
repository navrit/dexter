#version 330
uniform vec2 aspectRatio;
uniform vec2 offset;
uniform float zoom;

in vec2 position;
in vec2 texCoordsIn;
varying vec2 texCoords;

void main(void)
{
    texCoords = texCoordsIn;
    gl_Position = vec4(position*aspectRatio*zoom+offset,0,1);
}

