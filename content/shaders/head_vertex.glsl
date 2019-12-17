#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec4 vertexTCoord;

// std140 has an explicit memory layout. A bit wasteful if not careful, but will always be the same.
// float, int, bool = 4 bytes (N)
// vector = 2N or 4N
//  vec2 = 2N
//  vec3 = 4N
//  vec4 = 4N
// matrices
//  stored as array of column vectors. aligned as vec4 (4N)
// struct
//  padded to align with a multiple of 4N
// With the calculated offset values, based on the rules of the std140 layout, we can fill the buffer with the variable data at each offset using functions like glBufferSubData.
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
    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(vertexPosition, 1.0f);

    vertex.position = (model * vec4(vertexPosition, 1.0f)).xyz;
    vertex.normal = (model * vec4(vertexNormal, 0.0f)).xyz;
    vertex.color = vertexColor;
    vertex.tcoord = vertexTCoord;
}