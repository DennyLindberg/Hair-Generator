#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec4 vertexColor;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;    // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;          // 64 Column1, 80 Column2, 96 Column3, 112 Column4
};
uniform mat4 model;

out vec4 vcolor;

void main()
{
    gl_Position = projection * view * model * vec4(vertexPosition, 1.0f);
    vcolor = vertexColor;
}