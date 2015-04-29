#version 330
uniform vec2 aspectRatio;
uniform vec2 globalOffset;
uniform float zoom;
uniform int layer;

in vec2 verticesIn;
in vec2 textureCoordsIn;
in vec2 offsetsIn;
in vec2 orientationIn;

flat out int frameID;
smooth out vec2 c;

void main(void)
{
    //texCoords  = texelFetch(textureCoordsBuffer, 4*textureIndexIn+gl_VertexID).rg; //Grab the textureCoordinates for this vertex.
    c = textureCoordsIn*orientationIn;
    frameID = gl_InstanceID+layer;
    gl_Position = vec4((verticesIn+offsetsIn)*aspectRatio*zoom+globalOffset,0,1);
}

