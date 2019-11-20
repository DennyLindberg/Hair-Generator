#version 420 core

layout(lines) in;
layout(triangle_strip, max_vertices = 36) out;
const int vdivisions = 3;
const float tempheight = 0.01f;

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
  float startCurvatureHeight;
  float endCurvatureHeight;
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

bool ShouldFlipTriangle(vec3 start, vec3 end, vec3 topright, vec3 topleft)
{
    vec3 forward = end-start;
    vec3 opposite_dir = topright-topleft;
    return dot(forward, opposite_dir) > 0;
}

// bottomleft, bottomright, topright, topleft, bottomleft_worldspace, ...., normal1, normal2, texcoord1, texcoord2, FlipTriangleSplit
void EmitQuad(vec4 bl, vec4 br, vec4 tr, vec4 tl, vec4 bl_ws, vec4 br_ws, vec4 tr_ws, vec4 tl_ws, vec3 n1, vec3 n2, vec3 t1, vec3 t2, bool bFlip)
{
    // Triangle 1
    gl_Position = bl;
    vertex.normal = n1;
    vertex.position = bl_ws.xyz;
    vertex.color = bl_ws;
    vertex.tcoord = vec4(t1.b, t1.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = br;
    vertex.normal = n2;
    vertex.position = br_ws.xyz;
    vertex.color = br_ws;
    vertex.tcoord = vec4(t1.r, t1.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = bFlip? tl : tr;
    vertex.normal = n2;
    vertex.position = bFlip? tl_ws.xyz : tr_ws.xyz;
    vertex.color = bFlip? tl_ws : tr_ws;
    vertex.tcoord = bFlip? vec4(t2.b, t2.g, 0.0f, 1.0f) : vec4(t2.r, t2.g, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();

    // Triangle 2
    gl_Position = tr;
    vertex.normal = n2;
    vertex.position = tr_ws.xyz;
    vertex.color = tr_ws;
    vertex.tcoord = vec4(t2.r, t2.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = tl;
    vertex.normal = n1;
    vertex.position = tl_ws.xyz;
    vertex.color = tl_ws;
    vertex.tcoord = vec4(t2.b, t2.g, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = bFlip? br : bl;
    vertex.normal = n1;
    vertex.position = bFlip? br_ws.xyz : bl_ws.xyz;
    vertex.color = bFlip? br_ws : bl_ws;
    vertex.tcoord = bFlip? vec4(t1.r, t1.g, 0.0f, 1.0f) : vec4(t1.b, t1.g, 0.0f, 1.0f);
    EmitVertex();
    EndPrimitive();
}

void GenerateSingleQuad(SegmentData data)
{
    vec3 bottomleft  = data.start + data.startWidthVector;
    vec3 bottomright = data.start - data.startWidthVector;
    vec3 topright    = data.end   - data.endWidthVector;
    vec3 topleft     = data.end   + data.endWidthVector;
   
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

    bool bFlipTriangle = ShouldFlipTriangle(data.start, data.end, topright, topleft);
    EmitQuad(bottomleftt, bottomrightt, toprightt, topleftt, bottomleftws, bottomrightws, toprightws, topleftws, data.startNormal, data.endNormal, data.startTexcoord, data.endTexcoord, bFlipTriangle);
}

void GenerateDoubleQuad(SegmentData data)
{
    // base quad
    vec3 bottomleft  = data.start + data.startWidthVector;
    vec3 bottomright = data.start - data.startWidthVector;
    vec3 topright    = data.end   - data.endWidthVector;
    vec3 topleft     = data.end   + data.endWidthVector;

    // middle split
    vec3 bottommiddle = (bottomleft+bottomright)/2.0f;
    vec3 topmiddle = (topleft+topright)/2.0f;

    // apply height offset
    vec3 heightdirection1 = data.startNormal * tempheight/2.0f;
    vec3 heightdirection2 = data.endNormal * tempheight/2.0f;
    bottomleft -= heightdirection1;
    bottommiddle += heightdirection1;
    bottomright -= heightdirection1;
    topleft -= heightdirection2;
    topmiddle += heightdirection2;
    topright -= heightdirection2;

    // world space
    vec4 bottomleftws = model * vec4(bottomleft, 1.0f);
    vec4 bottommiddlews = model * vec4(bottommiddle, 1.0f);
    vec4 bottomrightws = model * vec4(bottomright, 1.0f);

    vec4 topleftws = model * vec4(topleft, 1.0f);
    vec4 topmiddlews = model * vec4(topmiddle, 1.0f);
    vec4 toprightws = model * vec4(topright, 1.0f);

    // clip space
    mat4 vp = projection * view;
    vec4 bottomleftt = vp * bottomleftws;
    vec4 bottommiddlet = vp * bottommiddlews;
    vec4 bottomrightt = vp * bottomrightws;

    vec4 topleftt = vp * topleftws;
    vec4 topmiddlet = vp * topmiddlews;
    vec4 toprightt = vp * toprightws;

    // texcoord
    vec3 texcoordmid1 = data.startTexcoord;
    vec3 texcoordmid2 = data.endTexcoord;
    texcoordmid1.r = (texcoordmid1.r + texcoordmid1.b)/2.0f;
    texcoordmid2.r = (texcoordmid2.r + texcoordmid2.b)/2.0f;

    bool bFlipTriangle = ShouldFlipTriangle(data.start, data.end, topright, topleft);
    EmitQuad(bottomleftt, bottommiddlet, topmiddlet, topleftt, bottomleftws, bottommiddlews, topmiddlews, topleftws, data.startNormal, data.endNormal, texcoordmid1, texcoordmid2, bFlipTriangle);

    // texcoord
    texcoordmid1 = data.startTexcoord;
    texcoordmid2 = data.endTexcoord;
    texcoordmid1.b = (texcoordmid1.r + texcoordmid1.b)/2.0f;
    texcoordmid2.b = (texcoordmid2.r + texcoordmid2.b)/2.0f;
    EmitQuad(bottommiddlet, bottomrightt, toprightt, topmiddlet, bottommiddlews, bottomrightws, toprightws, topmiddlews, data.startNormal, data.endNormal, texcoordmid1, texcoordmid2, bFlipTriangle);
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
    segment.startCurvatureHeight = tempheight;
    segment.startNormal = controlpoint[0].normal;
    segment.startTexcoord = controlpoint[0].texcoord;
    for (int i=1; i<vdivisions; i++)
    {
        // Endpoints
        t = i*timestep;
        segment.end = bezier(bcp1, bcp2, bcp3, bcp4, t);
        segment.endWidthVector = mix(startWidthVector, endWidthVector, t);
        segment.endCurvatureHeight = tempheight;
        segment.endNormal = normalize(mix(controlpoint[0].normal, controlpoint[1].normal, t));
        segment.endTexcoord = mix(controlpoint[0].texcoord, controlpoint[1].texcoord, t);

        // Generate
        GenerateDoubleQuad(segment);

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
    segment.endCurvatureHeight = tempheight;
    segment.endNormal = controlpoint[1].normal;
    segment.endTexcoord = controlpoint[1].texcoord;
    GenerateDoubleQuad(segment);
}