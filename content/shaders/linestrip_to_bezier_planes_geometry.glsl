#version 420 core

layout(lines) in;
layout(triangle_strip, max_vertices = 18) out;

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

void GenerateQuad(vec3 start, vec3 end, vec3 startWidthVector, vec3 endWidthVector, vec3 startNormal, vec3 endNormal, float vBegin, float vEnd, float uBegin1, float uBegin2, float uEnd1, float uEnd2)
{
    vec3 bottomleft  = start + startWidthVector;
    vec3 bottomright = start - startWidthVector;
    vec3 topright    = end   - endWidthVector;
    vec3 topleft     = end   + endWidthVector;

    // Determine triangle split (some triangles become really thin if this check is not done)
    vec3 forward = end-start;
    vec3 opposite_dir = topright-topleft;
    bool flip_triangle = dot(forward, opposite_dir) > 0;
   
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
    vertex.tcoord = vec4(uBegin2, vBegin, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = bottomrightt;
    vertex.normal = endNormal;
    vertex.position = bottomrightws.xyz;
    vertex.color = bottomrightws;
    vertex.tcoord = vec4(uBegin1, vBegin, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = flip_triangle? topleftt : toprightt;
    vertex.normal = endNormal;
    vertex.position = flip_triangle? topleftws.xyz : toprightws.xyz;
    vertex.color = flip_triangle? topleftws : toprightws;
    vertex.tcoord = flip_triangle? vec4(uEnd2, vEnd, 0.0f, 1.0f) : vec4(uEnd1, vEnd, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();

    // Triangle 2
    gl_Position = toprightt;
    vertex.normal = endNormal;
    vertex.position = toprightws.xyz;
    vertex.color = toprightws;
    vertex.tcoord = vec4(uEnd1, vEnd, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = topleftt;
    vertex.normal = startNormal;
    vertex.position = topleftws.xyz;
    vertex.color = topleftws;
    vertex.tcoord = vec4(uEnd2, vEnd, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = flip_triangle? bottomrightt : bottomleftt;
    vertex.normal = startNormal;
    vertex.position = flip_triangle? bottomrightws.xyz : bottomleftws.xyz;
    vertex.color = flip_triangle? bottomrightws : bottomleftws;
    vertex.tcoord = flip_triangle? vec4(uBegin1, vBegin, 0.0f, 1.0f) : vec4(uBegin2, vBegin, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 startWidthVector = controlpoint[0].bitangent*controlpoint[0].width;
    vec3 endWidthVector = controlpoint[1].bitangent*controlpoint[1].width;

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
    vec3 b1widthvec = mix(startWidthVector, endWidthVector, t1);

    vec3 b2normal = normalize(mix(controlpoint[0].normal, controlpoint[1].normal, t2));
    vec3 b2widthvec = mix(startWidthVector, endWidthVector, t2);

    // Compute v-coord based on distances to avoid uneven stretching between divisions
    float dist1 = distance(start, b1);
    float dist2 = distance(b1, b2);
    float dist3 = distance(b2, end);
    float total_dist = dist1 + dist2 + dist3;
    float v1 = dist1 / total_dist;
    float v2 = v1 + dist2 / total_dist;

    // Get actual uv-coordinates and lerp them
    float ustart1 = controlpoint[0].texcoord.r;
    float ustart2 = controlpoint[0].texcoord.b;
    float uend1 = controlpoint[1].texcoord.r;
    float uend2 = controlpoint[1].texcoord.b;
    
    float vstart = controlpoint[0].texcoord.g;
    float vend = controlpoint[1].texcoord.g;
    v1 = mix(vstart, vend, v1);
    v2 = mix(vstart, vend, v2);
    float u11 = mix(ustart1, uend1, v1);
    float u12 = mix(ustart2, uend2, v1);
    float u21 = mix(ustart1, uend1, v2);
    float u22 = mix(ustart2, uend2, v2);


    //GenerateQuad(start, end, startWidthVector, endWidthVector, controlpoint[0].normal, controlpoint[1].normal, 0.0f, 1.0f);


    GenerateQuad(start, b1, startWidthVector, b1widthvec, controlpoint[0].normal, b1normal, vstart, v1, ustart1, ustart2, u11, u12);
    GenerateQuad(b1, b2, b1widthvec, b2widthvec, b1normal, b2normal, v1, v2, u11, u12, u21, u22);
    GenerateQuad(b2, end, b2widthvec, endWidthVector, b2normal, controlpoint[1].normal, v2, vend, u21, u22, uend1, uend2);
}