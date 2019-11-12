#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in float vertexWidth;

out CPAttrib
{
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    float width;
} controlpoint;

void main()
{
    gl_Position = vec4(vertexPosition, 1.0f);
    controlpoint.normal = vertexNormal;
    controlpoint.tangent = vertexTangent;
    controlpoint.bitangent = normalize(cross(vertexNormal, vertexTangent));
    controlpoint.width = vertexWidth;
}