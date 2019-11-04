#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec4 vertexColor;

uniform mat4 mvp;

out vec4 vcolor;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0f);
    vcolor = vertexColor;
}