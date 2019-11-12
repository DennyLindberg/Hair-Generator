#version 420 core

layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;       // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;             // 64 Column1, 80 Column2, 96 Column3, 112 Column4
    vec3 camera_position;  // 128
};
uniform mat4 model;

in CPAttrib
{
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    float width;
} controlpoint[];

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

    vec3 bottomleft  = start + controlpoint[0].bitangent*controlpoint[0].width; // "bottom left"
    vec3 bottomright = start - controlpoint[0].bitangent*controlpoint[0].width; // "bottom right"
    vec3 topright    = end   - controlpoint[1].bitangent*controlpoint[1].width; // "top right"
    vec3 topleft     = end   + controlpoint[1].bitangent*controlpoint[1].width; // "top left"

    vec4 uv00 = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 uv01 = vec4(0.0, 1.0, 0.0, 1.0);
    vec4 uv11 = vec4(1.0, 1.0, 0.0, 1.0);
    vec4 uv10 = vec4(1.0, 0.0, 0.0, 1.0);
    
    // world space
    vec4 bottomleftws = model * vec4(bottomleft, 1.0);
    vec4 bottomrightws = model * vec4(bottomright, 1.0);
    vec4 toprightws = model * vec4(topright, 1.0);
    vec4 topleftws = model * vec4(topleft, 1.0);

    // clip space
    mat4 vp = projection * view;
    vec4 bottomleftt = vp * bottomleftws;
    vec4 bottomrightt = vp * bottomrightws;
    vec4 toprightt = vp * toprightws;
    vec4 topleftt = vp * topleftws;

    // Triangle 1
    gl_Position = bottomleftt;
    vertex.normal = controlpoint[0].normal;
    vertex.position = bottomleftws.xyz;
    vertex.color = bottomleftws;
    vertex.tcoord = uv10;
    EmitVertex();
    gl_Position = bottomrightt;
    vertex.normal = controlpoint[1].normal;
    vertex.position = bottomrightws.xyz;
    vertex.color = bottomrightws;
    vertex.tcoord = uv00;
    EmitVertex();
    gl_Position = toprightt;
    vertex.normal = controlpoint[1].normal;
    vertex.position = toprightws.xyz;
    vertex.color = toprightws;
    vertex.tcoord = uv01;
    EmitVertex();
    EndPrimitive();

    // Triangle 2
    gl_Position = toprightt;
    vertex.normal = controlpoint[1].normal;
    vertex.position = toprightws.xyz;
    vertex.color = toprightws;
    vertex.tcoord = uv01;
    EmitVertex();
    gl_Position = topleftt;
    vertex.normal = controlpoint[0].normal;
    vertex.position = topleftws.xyz;
    vertex.color = topleftws;
    vertex.tcoord = uv11;
    EmitVertex();
    gl_Position = bottomleftt;
    vertex.normal = controlpoint[0].normal;
    vertex.position = bottomleftws.xyz;
    vertex.color = bottomleftws;
    vertex.tcoord = uv10;
    EmitVertex();
    EndPrimitive();
}