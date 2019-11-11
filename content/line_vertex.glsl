#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec4 vertexColor;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;       // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;             // 64 Column1, 80 Column2, 96 Column3, 112 Column4
    vec3 camera_position;  // 128
};
uniform mat4 model;

uniform bool transformVerticesInVertexShader = true;
uniform bool useUniformColor = false;
uniform vec4 uniformColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

// World space attributes
out VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex;

void main()
{
    gl_Position = transformVerticesInVertexShader? projection * view * model * vec4(vertexPosition, 1.0f) : vec4(vertexPosition, 1.0f);
    vertex.position = transformVerticesInVertexShader? (model * gl_Position).xyz : gl_Position.xyz;
    vertex.color = useUniformColor? uniformColor : vertexColor;
}