#version 420 core

layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;    // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;          // 64 Column1, 80 Column2, 96 Column3, 112 Column4
};
uniform mat4 model;

// World space attributes
in VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} linepoint[];

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
    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 tangent = end-start;
    vec3 bitangent = normalize(cross(up, tangent));

    // Points are computed by "taking a step" from each previous point
    float width = 0.1;
    vec3 p1 = start - bitangent*width; // "bottom left"
    vec3 p2 = p1 + tangent;            // "bottom right"
    vec3 p3 = p2 + 2.0*bitangent*width;// "top right"
    vec3 p4 = p3 - tangent;            // "top left"

    
    // world space
    vec4 p1ws = model * vec4(p1, 1.0);
    vec4 p2ws = model * vec4(p2, 1.0);
    vec4 p3ws = model * vec4(p3, 1.0);
    vec4 p4ws = model * vec4(p4, 1.0);

    // clip space
    mat4 vp = projection * view;
    vec4 p1t = vp * p1ws;
    vec4 p2t = vp * p2ws;
    vec4 p3t = vp * p3ws;
    vec4 p4t = vp * p4ws;

    vertex.normal = up;
    vertex.color = linepoint[0].color;
    vertex.tcoord = linepoint[0].color;

    // Triangle 1
    gl_Position = p1t;
    vertex.position = p1ws.xyz;
    EmitVertex();
    gl_Position = p2t;
    vertex.position = p2ws.xyz;
    EmitVertex();
    gl_Position = p3t;
    vertex.position = p3ws.xyz;
    EmitVertex();
    EndPrimitive();

    // Triangle 2
    gl_Position = p3t;
    vertex.position = p3ws.xyz;
    EmitVertex();
    gl_Position = p4t;
    vertex.position = p4ws.xyz;
    EmitVertex();
    gl_Position = p1t;
    vertex.position = p1ws.xyz;
    EmitVertex();
    EndPrimitive();
}