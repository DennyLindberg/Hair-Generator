#version 420 core

layout(lines) in;
layout(line_strip, max_vertices = 12) out; // 6 for coordinate axis + 2n segments

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

vec3 bezier(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float t)
{
    vec3 sub1 = mix(p1, p2, t);
    vec3 sub2 = mix(p2, p3, t);
    vec3 sub3 = mix(p3, p4, t);

    vec3 subsub1 = mix(sub1, sub2, t);
    vec3 subsub2 = mix(sub2, sub3, t);

    return mix(subsub1, subsub2, t);
}

void GenerateQuad(vec3 start, vec3 end, vec3 startWidthVector, vec3 endWidthVector, vec3 startNormal, vec3 endNormal, float vBegin, float vEnd)
{
    vec3 bottomleft  = start + startWidthVector;
    vec3 bottomright = start - startWidthVector;
    vec3 topright    = end   - endWidthVector;
    vec3 topleft     = end   + endWidthVector;
   
    // world space
    vec4 bottomleftws = model * vec4(bottomleft, 1.0f);
    vec4 bottomrightws = model * vec4(bottomright, 1.0f);
    vec4 toprightws = model * vec4(topright, 1.0f);
    vec4 topleftws = model * vec4(topleft, 1.0f);

    // clip space
    mat4 vp = projection * view;
    vec4 bottomleftt = vp * bottomleftws;
    vec4 bottomrightt = vp * bottomrightws;
    vec4 toprightt = vp * toprightws;
    vec4 topleftt = vp * topleftws;

    // Triangle 1
    gl_Position = bottomleftt;
    vertex.normal = startNormal;
    vertex.position = bottomleftws.xyz;
    vertex.color = bottomleftws;
    vertex.tcoord = vec4(1.0f, vBegin, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = bottomrightt;
    vertex.normal = endNormal;
    vertex.position = bottomrightws.xyz;
    vertex.color = bottomrightws;
    vertex.tcoord = vec4(0.0f, vBegin, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = toprightt;
    vertex.normal = endNormal;
    vertex.position = toprightws.xyz;
    vertex.color = toprightws;
    vertex.tcoord = vec4(0.0f, vEnd, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();

    // Triangle 2
    gl_Position = toprightt;
    vertex.normal = endNormal;
    vertex.position = toprightws.xyz;
    vertex.color = toprightws;
    vertex.tcoord = vec4(0.0f, vEnd, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = topleftt;
    vertex.normal = startNormal;
    vertex.position = topleftws.xyz;
    vertex.color = topleftws;
    vertex.tcoord = vec4(1.0f, vEnd, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = bottomleftt;
    vertex.normal = startNormal;
    vertex.position = bottomleftws.xyz;
    vertex.color = bottomleftws;
    vertex.tcoord = vec4(1.0f, vBegin, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();
}

void EmitSegment(vec3 start, vec3 end, vec3 normal1, vec3 normal2, vec4 color, float vcoord1, float vcoord2)
{
    // world space
    vec4 startws = model * vec4(start, 1.0f);
    vec4 endws = model * vec4(end, 1.0f);

    // clip space
    mat4 vp = projection * view;
    vec4 startcs = vp * startws;
    vec4 endcs = vp * endws;

    gl_Position = startcs;
    vertex.normal = normal1;
    vertex.position = startws.xyz;
    vertex.color = color;
    vertex.tcoord = vec4(0.0f, vcoord1, 0.0f, 1.0f);
    EmitVertex();

    gl_Position = endcs;
    vertex.normal = normal2;
    vertex.position = endws.xyz;
    vertex.color = color;
    vertex.tcoord = vec4(0.0f, vcoord2, 0.0f, 1.0f);
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

    EmitSegment(
        origin, origin+x, 
        z, z, 
        red, 
        0.0f, 1.0f
    );
    EmitSegment(
        origin, origin+y, 
        z, z, 
        green, 
        0.0f, 1.0f
    );
    EmitSegment(
        origin, origin+z, 
        x, x, 
        blue, 
        0.0f, 1.0f
    );
}

void main()
{
    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 startWidthVector = controlpoint[0].bitangent*controlpoint[0].width;
    vec3 endWidthVector = controlpoint[1].bitangent*controlpoint[1].width;
    float vBegin = 0.0f;
    float vEnd = 1.0f;

    // 4 points
    vec3 p1 = start;
    vec3 p2 = start + controlpoint[0].tangent;
    vec3 p3 = end - controlpoint[1].tangent;
    vec3 p4 = end;

    // Get bezier points
    float t1 = 0.33f;
    float t2 = 0.66f;
    vec3 b1 = bezier(p1, p2, p3, p4, t1);
    vec3 b2 = bezier(p1, p2, p3, p4, t2);

    vec3 b1normal = normalize(mix(controlpoint[0].normal, controlpoint[1].normal, t1));
    vec3 b2normal = normalize(mix(controlpoint[0].normal, controlpoint[1].normal, t2));

    // Compute v-coord based on distances to avoid uneven stretching between divisions
    float dist1 = distance(start, b1);
    float dist2 = distance(b1, b2);
    float dist3 = distance(b2, end);
    float total_dist = dist1 + dist2 + dist3;
    float v1 = dist1 / total_dist;
    float v2 = v1 + dist2 / total_dist;

    // TODO: How to draw only the end point when we reach the end of the line strip? 
    //       Maybe it's okay to draw duplicates for this shader as it only involves lines...
    EmitCoordinateFrame(start, 0);
    EmitCoordinateFrame(end, 1);

    // Lines
    vec4 white = vec4(1.0f);
    EmitSegment(
        start, b1, 
        controlpoint[0].normal, b1normal, 
        white, 
        0.0f, v1
    );
    EmitSegment(
        b1, b2, 
        b1normal, b2normal, 
        white, 
        v1, v2
    );
    EmitSegment(
        b2, end, 
        b2normal, controlpoint[1].normal, 
        white, 
        v2, 1.0f
    );

    // TODO:
    // Generate more segments
}