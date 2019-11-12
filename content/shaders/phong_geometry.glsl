#version 420 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// World space attributes
in VertexAttrib
{
    vec3 position; // discarded
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex[];

// World space attributes
out VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertexout;

void main()
{
    // Flat face shading
    vec3 avgNormal = vec3(0.0);
    for (int i=0; i<gl_in.length(); i++)
    {
        avgNormal += vertex[i].normal;
    }
    avgNormal /= gl_in.length();

    // Update outputs to fragment shader
    for (int i=0; i<gl_in.length(); i++)
    {
        gl_Position = gl_in[i].gl_Position;

        vertexout.position = gl_in[i].gl_Position.xyz;
        //vertexout.normal = avgNormal;
        vertexout.normal = vertex[i].normal;
        vertexout.color = vertex[i].color;
        vertexout.tcoord = vertex[i].tcoord;
        EmitVertex();
    }
    EndPrimitive();
}