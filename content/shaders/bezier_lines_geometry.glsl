#version 420 core

layout(lines) in;
layout(line_strip, max_vertices = 20) out; // 6*2 for start/end coordinate axis (x2, start/end) + 2n segments (max 4 subdivisions)

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
    vec3 texcoord;
    float width;
    float thickness;
    int shape;
    int subdivisions; // max 4
} controlpoint[];

// World space attributes
out VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex;

vec3 bezier(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float t)
{
    vec3 sub1 = mix(p1, p2, t);
    vec3 sub2 = mix(p2, p3, t);
    vec3 sub3 = mix(p3, p4, t);

    vec3 subsub1 = mix(sub1, sub2, t);
    vec3 subsub2 = mix(sub2, sub3, t);

    return mix(subsub1, subsub2, t);
}

void EmitSegment(vec3 start, vec3 end, vec4 color)
{
    // world space
    vec4 startws = model * vec4(start, 1.0f);
    vec4 endws = model * vec4(end, 1.0f);

    // clip space
    mat4 vp = projection * view;
    vec4 startcs = vp * startws;
    vec4 endcs = vp * endws;

    gl_Position = startcs;
    vertex.position = startws.xyz;
    vertex.color = color;
    EmitVertex();

    gl_Position = endcs;
    vertex.position = endws.xyz;
    vertex.color = color;
    EmitVertex();

    EndPrimitive();
}

void EmitCoordinateFrame(vec3 origin, int cp_index)
{
    vec4 red = vec4(1.0f, 0.0f, 0.0f, 1.0f);    
    vec4 green = vec4(0.0f, 1.0f, 0.0f, 1.0f);    
    vec4 blue = vec4(0.0f, 0.0f, 1.0f, 1.0f);    

    // Coordinate frame
    // tangent = "x"
    vec3 x = controlpoint[cp_index].tangent;
    vec3 y = controlpoint[cp_index].bitangent * controlpoint[cp_index].width;
    vec3 z = controlpoint[cp_index].normal * controlpoint[cp_index].width;    // scale down normal to same size as bitangent (otherwise unnecessary size in viewport)

    EmitSegment(origin, origin+x, red);
    EmitSegment(origin, origin+y, green);
    EmitSegment(origin, origin+z, blue);
}

void main()
{
    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;

    // Bezier control points
    vec3 bcp1 = start;
    vec3 bcp2 = start + controlpoint[0].tangent;
    vec3 bcp3 = end - controlpoint[1].tangent;
    vec3 bcp4 = end;

    // TODO: How to draw only the end point when we reach the end of the line strip? 
    //       Maybe it's okay to draw duplicates for this shader as it only involves lines...
    EmitCoordinateFrame(start, 0); // 6 points
    EmitCoordinateFrame(end, 1);   // 6 points

    // 2 points * subdivisions
    vec4 white = vec4(1.0f);
    vec3 a = start;
    vec3 b;
    if (controlpoint[0].subdivisions > 0)
    {
        float timestep = 1.0f/controlpoint[0].subdivisions;
        for (int i=1; i<controlpoint[0].subdivisions; i++)
        {
            float t = i*timestep;
            b = bezier(bcp1, bcp2, bcp3, bcp4, t);
            EmitSegment(a, b, white);

            a = b;
        }
    }
    b = end;
    EmitSegment(a, b, white);
}