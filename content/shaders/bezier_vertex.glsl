#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexTexcoord;
layout(location = 4) in float vertexWidth;
layout(location = 5) in float vertexThickness;
layout(location = 6) in int vertexSegmentShape;
layout(location = 7) in int vertexSegmentSubdivisions;

uniform int shapeOverride = -1;
uniform int subdivisionsOverride = -1;

out CPAttrib
{
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 texcoord;
    float width;
    float thickness;
    int shape;
    int subdivisions;
} controlpoint;

void main()
{
    gl_Position = vec4(vertexPosition, 1.0f);
    controlpoint.normal = vertexNormal;
    controlpoint.tangent = vertexTangent;
    controlpoint.bitangent = normalize(cross(vertexNormal, vertexTangent));
    controlpoint.texcoord = vertexTexcoord;
    controlpoint.width = vertexWidth;
    controlpoint.thickness = vertexThickness;
    controlpoint.shape = (shapeOverride >= 0)? shapeOverride : vertexSegmentShape;
    controlpoint.subdivisions = (subdivisionsOverride >= 0)? subdivisionsOverride : vertexSegmentSubdivisions;
}