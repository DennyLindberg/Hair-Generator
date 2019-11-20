#version 420 core

layout(lines) in;
layout(triangle_strip, max_vertices = 18) out;
const int vdivisions = 3;

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
    // height/curvature
} controlpoint[];

// World space attributes
out VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} vertex;

// This data is used to define a strip of hair
struct SegmentData
{
  vec3 start;
  vec3 end;
  vec3 startWidthVector;
  vec3 endWidthVector;
  vec3 startCurvatureHeight;
  vec3 endCurvatureHeight;
  vec3 startNormal;
  vec3 endNormal;
  vec3 startTexcoord; // ustart1, vstart, ustart2
  vec3 endTexcoord;   // uend1, vend, uend2
};

vec3 bezier(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float t)
{
    vec3 sub1 = mix(p1, p2, t);
    vec3 sub2 = mix(p2, p3, t);
    vec3 sub3 = mix(p3, p4, t);

    vec3 subsub1 = mix(sub1, sub2, t);
    vec3 subsub2 = mix(sub2, sub3, t);

    return mix(subsub1, subsub2, t);
}

void GenerateQuad(SegmentData data)
{
    vec3 bottomleft  = data.start + data.startWidthVector;
    vec3 bottomright = data.start - data.startWidthVector;
    vec3 topright    = data.end   - data.endWidthVector;
    vec3 topleft     = data.end   + data.endWidthVector;

    // Determine triangle split (some triangles become really thin if this check is not done)
    vec3 forward = data.end-data.start;
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
    vertex.normal = data.startNormal;
    vertex.position = bottomleftws.xyz;
    vertex.color = bottomleftws;
    vertex.tcoord = vec4(data.startTexcoord.b, data.startTexcoord.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = bottomrightt;
    vertex.normal = data.endNormal;
    vertex.position = bottomrightws.xyz;
    vertex.color = bottomrightws;
    vertex.tcoord = vec4(data.startTexcoord.r, data.startTexcoord.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = flip_triangle? topleftt : toprightt;
    vertex.normal = data.endNormal;
    vertex.position = flip_triangle? topleftws.xyz : toprightws.xyz;
    vertex.color = flip_triangle? topleftws : toprightws;
    vertex.tcoord = flip_triangle? vec4(data.endTexcoord.b, data.endTexcoord.g, 0.0f, 1.0f) : vec4(data.endTexcoord.r, data.endTexcoord.g, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();

    // Triangle 2
    gl_Position = toprightt;
    vertex.normal = data.endNormal;
    vertex.position = toprightws.xyz;
    vertex.color = toprightws;
    vertex.tcoord = vec4(data.endTexcoord.r, data.endTexcoord.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = topleftt;
    vertex.normal = data.startNormal;
    vertex.position = topleftws.xyz;
    vertex.color = topleftws;
    vertex.tcoord = vec4(data.endTexcoord.b, data.endTexcoord.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = flip_triangle? bottomrightt : bottomleftt;
    vertex.normal = data.startNormal;
    vertex.position = flip_triangle? bottomrightws.xyz : bottomleftws.xyz;
    vertex.color = flip_triangle? bottomrightws : bottomleftws;
    vertex.tcoord = flip_triangle? vec4(data.startTexcoord.r, data.startTexcoord.g, 0.0f, 1.0f) : vec4(data.startTexcoord.b, data.startTexcoord.g, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 startWidthVector = controlpoint[0].bitangent*controlpoint[0].width;
    vec3 endWidthVector = controlpoint[1].bitangent*controlpoint[1].width;

    // Bezier control points
    vec3 bcp1 = start;
    vec3 bcp2 = start + controlpoint[0].tangent;
    vec3 bcp3 = end - controlpoint[1].tangent;
    vec3 bcp4 = end;

    float timestep = 1.0f/vdivisions;
    float t = timestep;
    SegmentData segment;
    segment.start = start;
    segment.startWidthVector = startWidthVector;
    //segment.startCurvatureHeight
    segment.startNormal = controlpoint[0].normal;
    segment.startTexcoord = controlpoint[0].texcoord;
    for (int i=1; i<vdivisions; i++)
    {
        // Endpoints
        t = i*timestep;
        segment.end = bezier(bcp1, bcp2, bcp3, bcp4, t);
        segment.endWidthVector = mix(startWidthVector, endWidthVector, t);
        //segment.endCurvatureHeight
        segment.endNormal = normalize(mix(controlpoint[0].normal, controlpoint[1].normal, t));
        segment.endTexcoord = mix(controlpoint[0].texcoord, controlpoint[1].texcoord, t);

        // Generate
        GenerateQuad(segment);

        // Update startpoints for next iteration
        segment.start = segment.end;
        segment.startWidthVector = segment.endWidthVector;
        segment.startCurvatureHeight = segment.endCurvatureHeight;
        segment.startNormal = segment.endNormal;
        segment.startTexcoord = segment.endTexcoord;
    }

    // Generate last segment
    segment.end = end;
    segment.endWidthVector = endWidthVector;
    //segment.endCurvatureHeight
    segment.endNormal = controlpoint[1].normal;
    segment.endTexcoord = controlpoint[1].texcoord;
    GenerateQuad(segment);
}