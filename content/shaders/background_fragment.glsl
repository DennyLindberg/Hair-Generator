#version 330

layout(location = 0) out vec4 color;

in vec3 vPosition;
in vec3 vNormal;
in vec4 vColor;
in vec4 vTCoord;

void main() 
{
    vec4 color1 = vec4(0.5, 0.8, 0.9, 1.0);
    vec4 color2 = vec4(0.1, 0.2, 0.3, 1.0);
    color = mix(color1, color2, vTCoord.y);
}