#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec4 vertexTCoord;

uniform mat4 mvp;

out VertexAttrib
{
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0f);

    vertex.normal = vertexNormal;
    vertex.color = vertexColor;
    vertex.tcoord = vertexTCoord;
}