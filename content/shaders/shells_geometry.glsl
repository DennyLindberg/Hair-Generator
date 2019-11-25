#version 420 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out; // emit the same triangle, OR additional ones for each shell!

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;       // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;             // 64 Column1, 80 Column2, 96 Column3, 112 Column4
    vec3 camera_position;  // 128
};
uniform mat4 model;

// World space attributes
in VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex_in[];

// World space attributes
out VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex_out;

void main()
{
    /*
        passthrough original triangle
    */
    float alphasum = 0.0f;
    for (int i=0; i<3; i++)
    {
        gl_Position = gl_in[i].gl_Position;
        vertex_out.position = vertex_in[i].position; 
        vertex_out.normal = vertex_in[i].normal;
        vertex_out.color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        vertex_out.tcoord = vertex_in[i].tcoord;
        EmitVertex();

        alphasum += vertex_in[i].color.r;
    }
    EndPrimitive();

    /*
        determine if the shell should be generated
    */
    if (alphasum > 0.0f)
    {
        for (int i=0; i<3; i++)
        {
            float dist = 0.003f * vertex_in[i].color.r; // only offset the plane if the vertex has color (to allow transition of the shell into the head)
            vec3 offset = dist * vertex_in[i].normal;
            vec4 offset_position = vec4(vertex_in[i].position + offset, 1.0f);

            gl_Position = projection * offset_position;
            vertex_out.position = offset_position.xyz; 
            vertex_out.normal = vertex_in[i].normal;
            vertex_out.color = vertex_in[i].color; //vec4(1.0f, 0.0f, 0.0f, 1.0f); // fill triangle with same color (not all vertices have a fill)
            vertex_out.tcoord = vertex_in[i].tcoord;
            EmitVertex();
        }
        EndPrimitive();
    }
}