#version 330

layout(location = 0) out vec4 color;

in VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} fragment;

void main() 
{
    color = fragment.color;
}