#version 330

uniform sampler2D textureSampler;
layout(location = 0) out vec4 color;

in vec3 vPosition;
in vec3 vNormal;
in vec4 vColor;
in vec4 vTCoord;

void main() 
{
    color = texture(textureSampler, vTCoord.rg);
}