#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec4 vertexTCoord;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;       // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;             // 64 Column1, 80 Column2, 96 Column3, 112 Column4
    vec3 camera_position;  // 128
};
uniform mat4 model;

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
    mat4 mv = view * model;
    vec4 pos = mv * vec4(vertexPosition, 1.0f);
    gl_Position = projection * pos;
    vertex.position = pos.xyz;
    vertex.normal = normalize((mv * vec4(vertexNormal, 0.0f)).xyz);
    float blend = smoothstep(0.32f, 0.35f, vertexPosition.y);
    vertex.color = mix(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), blend);
    vertex.tcoord = vertexTCoord;
}