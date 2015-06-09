#version 330
uniform vec2 aspectRatio;
uniform vec2 globalOffset;
uniform float zoom;
uniform int layer;

in vec2 verticesIn;
in vec2 textureCoordsIn;
in vec2 offsetsIn;
in vec4 orientationIn;

flat out int frameID;
smooth out vec2 c;

void main(void)
{
    //texCoords  = texelFetch(textureCoordsBuffer, 4*textureIndexIn+gl_VertexID).rg; //Grab the textureCoordinates for this vertex.
    c = vec2(textureCoordsIn.x*orientationIn.xy+textureCoordsIn.y*orientationIn.zw);
    frameID = gl_InstanceID+layer;
    gl_Position = vec4((verticesIn+offsetsIn+globalOffset)*aspectRatio*zoom,0,1);
}

