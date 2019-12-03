#version 420 core

layout(lines) in;
layout(triangle_strip, max_vertices = 72) out; // max segments 4 (subdivisions) as each segment has 6 to 18 vertices. Hardware can only emit 73 vertices in total

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

// This data is used to define a strip of hair
struct SegmentData
{
    int shape;
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

struct PointData
{
    vec4 position;
    vec4 position_ws;
    vec3 normal;
    vec2 texcoord;
};

struct QuadData
{
    PointData p1;
    PointData p2;
    PointData p3;
    PointData p4;
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
    bFlip = false;

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

void EmitPointData(PointData p)
{
    gl_Position = p.position;
    vertex.normal = p.normal;
    vertex.position = p.position_ws.xyz;
    vertex.color = p.position_ws;
    vertex.tcoord = vec4(p.texcoord.r, p.texcoord.g, 0.0f, 1.0f);
    EmitVertex();
}

void EmitTriangle(PointData p1, PointData p2, PointData p3)
{
    EmitPointData(p1);
    EmitPointData(p2);
    EmitPointData(p3);
    EndPrimitive();
}

void GenerateSingleQuad(SegmentData data)
{
    /*
        p4 --- end --- p3
                      / |
                   /    |
                /       |
             /          |
          /             |
        p1 ---start--- p2
    */

    PointData p1;
    PointData p2;
    PointData p3;
    PointData p4;

    p1.normal = data.startNormal;
    p2.normal = data.startNormal;
    p3.normal = data.endNormal;
    p4.normal = data.endNormal;

    // texcoord.rgb = (ustart, v, uend)
    p1.texcoord = vec2(data.startTexcoord.r, data.startTexcoord.g);
    p2.texcoord = vec2(data.startTexcoord.b, data.startTexcoord.g);
    p3.texcoord = vec2(data.endTexcoord.b, data.endTexcoord.g);
    p4.texcoord = vec2(data.endTexcoord.r, data.endTexcoord.g);

    // Localspace
    p1.position = vec4(data.start + data.startWidthVector, 1.0f);
    p2.position = vec4(data.start - data.startWidthVector, 1.0f);
    p3.position = vec4(data.end   - data.endWidthVector, 1.0f);
    p4.position = vec4(data.end   + data.endWidthVector, 1.0f);
    bool bFlipTriangle = ShouldFlipTriangle(data.start, data.end, p3.position.xyz, p4.position.xyz);
    
    // Compute world space
    // width vector points "left" to p1 and p4
    p1.position_ws = model * p1.position;
    p2.position_ws = model * p2.position;
    p3.position_ws = model * p3.position;
    p4.position_ws = model * p4.position;

    // Project into clip space
    mat4 vp = projection * view;
    p1.position = vp * p1.position_ws;
    p2.position = vp * p2.position_ws;
    p3.position = vp * p3.position_ws;
    p4.position = vp * p4.position_ws;
    
    if (bFlipTriangle)
    {
        EmitTriangle(p1, p2, p4);
        EmitTriangle(p2, p3, p4);
    }
    else
    {
        EmitTriangle(p1, p2, p3);
        EmitTriangle(p3, p4, p1);
    }
}

void GenerateDoubleQuad(SegmentData data)
{
    /*
        p6 ----------- p5 ----------- p4
                      / |            /|
                   /    |          /  |
                /       |       /     |
             /          |    /        |
          /             | /           |
        p1 ----------- p2 ---------- p3
    */

    PointData p1;
    PointData p2;
    PointData p3;
    PointData p4;
    PointData p5;
    PointData p6;

    p1.normal = data.startNormal;
    p2.normal = data.startNormal;
    p3.normal = data.startNormal;
    p4.normal = data.endNormal;
    p5.normal = data.endNormal;
    p6.normal = data.endNormal;

    // texcoord.rgb = (ustart, v, uend)
    p1.texcoord = vec2(data.startTexcoord.r, data.startTexcoord.g);
    p2.texcoord = vec2((data.startTexcoord.r+data.startTexcoord.b)/2.0f, data.startTexcoord.g);
    p3.texcoord = vec2(data.startTexcoord.b, data.startTexcoord.g);
    p4.texcoord = vec2(data.endTexcoord.b, data.endTexcoord.g);
    p5.texcoord = vec2((data.endTexcoord.r+data.endTexcoord.b)/2.0f, data.endTexcoord.g);
    p6.texcoord = vec2(data.endTexcoord.r, data.endTexcoord.g);

    // Localspace
    p1.position = vec4(data.start + data.startWidthVector, 1.0f);
    p2.position = vec4(data.start, 1.0f);                           // midpoint
    p3.position = vec4(data.start - data.startWidthVector, 1.0f);
    p4.position = vec4(data.end - data.endWidthVector, 1.0f);
    p5.position = vec4(data.end, 1.0f);                             // midpoint
    p6.position = vec4(data.end + data.endWidthVector, 1.0f);
    bool bFlipTriangle1 = ShouldFlipTriangle(((p1.position+p2.position)/2.0).xyz, ((p5.position+p6.position)/2.0).xyz, p5.position.xyz, p6.position.xyz);
    bool bFlipTriangle2 = ShouldFlipTriangle(((p2.position+p3.position)/2.0).xyz, ((p4.position+p5.position)/2.0).xyz, p4.position.xyz, p5.position.xyz);

    // Apply thickness
    vec4 startThicknessOffset = vec4(data.startNormal * data.startCurvatureHeight / 2.0f, 0.0f);
    vec4 endThicknessOffset = vec4(data.endNormal * data.endCurvatureHeight / 2.0f, 0.0f);
    p1.position -= startThicknessOffset;
    p2.position += startThicknessOffset;
    p3.position -= startThicknessOffset;
    p4.position -= endThicknessOffset;
    p5.position += endThicknessOffset;
    p6.position -= endThicknessOffset;
    
    // Compute world space
    // width vector points "left" to p1 and p4
    p1.position_ws = model * p1.position;
    p2.position_ws = model * p2.position;
    p3.position_ws = model * p3.position;
    p4.position_ws = model * p4.position;
    p5.position_ws = model * p5.position;
    p6.position_ws = model * p6.position;

    // Project into clip space
    mat4 vp = projection * view;
    p1.position = vp * p1.position_ws;
    p2.position = vp * p2.position_ws;
    p3.position = vp * p3.position_ws;
    p4.position = vp * p4.position_ws;
    p5.position = vp * p5.position_ws;
    p6.position = vp * p6.position_ws;
    
    if (bFlipTriangle1)
    {
        EmitTriangle(p1, p2, p6);
        EmitTriangle(p6, p2, p5);
    }
    else
    {
        EmitTriangle(p1, p2, p5);
        EmitTriangle(p5, p6, p1);
    }
        
    if (bFlipTriangle2)
    {
        EmitTriangle(p2, p3, p5);
        EmitTriangle(p5, p3, p4);
    }
    else
    {
        EmitTriangle(p2, p3, p4);
        EmitTriangle(p4, p5, p2);
    }
}

void GenerateTripleQuad(SegmentData data)
{
    // base quad
    vec3 bottomleft  = data.start + data.startWidthVector;
    vec3 bottomright = data.start - data.startWidthVector;
    vec3 topright    = data.end   - data.endWidthVector;
    vec3 topleft     = data.end   + data.endWidthVector;

    // middle split
    vec3 bottommiddle = (bottomleft+bottomright)/2.0f;
    vec3 topmiddle = (topleft+topright)/2.0f;
    vec3 bottommiddlel = (bottommiddle + bottomleft)/2.0f;
    vec3 bottommiddler = (bottommiddle + bottomright)/2.0f;
    vec3 topmiddlel = (topmiddle + topleft)/2.0f;
    vec3 topmiddler = (topmiddle + topright)/2.0f;

    // apply height offset
    vec3 heightdirection1 = data.startNormal * data.startCurvatureHeight/2.0f;
    vec3 heightdirection2 = data.endNormal * data.endCurvatureHeight/2.0f;
    bottomleft -= heightdirection1;
    bottommiddlel += heightdirection1;
    bottommiddler += heightdirection1;
    bottomright -= heightdirection1;
    topleft -= heightdirection2;
    topmiddlel += heightdirection2;
    topmiddler += heightdirection2;
    topright -= heightdirection2;

    // world space
    vec4 bottomleftws = model * vec4(bottomleft, 1.0f);
    vec4 bottommiddlelws = model * vec4(bottommiddlel, 1.0f);
    vec4 bottommiddlerws = model * vec4(bottommiddler, 1.0f);
    vec4 bottomrightws = model * vec4(bottomright, 1.0f);

    vec4 topleftws = model * vec4(topleft, 1.0f);
    vec4 topmiddlelws = model * vec4(topmiddlel, 1.0f);
    vec4 topmiddlerws = model * vec4(topmiddler, 1.0f);
    vec4 toprightws = model * vec4(topright, 1.0f);

    // clip space
    mat4 vp = projection * view;
    vec4 bottomleftt = vp * bottomleftws;
    vec4 bottommiddlelt = vp * bottommiddlelws;
    vec4 bottommiddlert = vp * bottommiddlerws;
    vec4 bottomrightt = vp * bottomrightws;

    vec4 topleftt = vp * topleftws;
    vec4 topmiddlelt = vp * topmiddlelws;
    vec4 topmiddlert = vp * topmiddlerws;
    vec4 toprightt = vp * toprightws;

    // Quad 1
    vec3 texcoordmid1 = data.startTexcoord;
    vec3 texcoordmid2 = data.endTexcoord;
    texcoordmid1.r = mix(texcoordmid1.b, texcoordmid1.r, 0.25f);
    texcoordmid2.r = mix(texcoordmid2.b, texcoordmid2.r, 0.25f);
    bool bFlipTriangle = ShouldFlipTriangle(data.start, data.end, topmiddlel, topleft);
    EmitQuad(bottomleftt, bottommiddlelt, topmiddlelt, topleftt, bottomleftws, bottommiddlelws, topmiddlelws, topleftws, data.startNormal, data.endNormal, texcoordmid1, texcoordmid2, bFlipTriangle);

    // Quad 2
    texcoordmid1.b = texcoordmid1.r;
    texcoordmid2.b = texcoordmid2.r;
    texcoordmid1.r = mix(data.startTexcoord.b, data.startTexcoord.r, 0.75f);
    texcoordmid2.r = mix(data.endTexcoord.b, data.endTexcoord.r, 0.75f);
    bFlipTriangle = ShouldFlipTriangle(data.start, data.end, topmiddlel, topleft);
    EmitQuad(bottommiddlelt, bottommiddlert, topmiddlert, topmiddlelt, bottommiddlelws, bottommiddlerws, topmiddlerws, topmiddlelws, data.startNormal, data.endNormal, texcoordmid1, texcoordmid2, bFlipTriangle);

    // Quad 3
    texcoordmid1.b = texcoordmid1.r;
    texcoordmid2.b = texcoordmid2.r;
    texcoordmid1.r = data.startTexcoord.r;
    texcoordmid2.r = data.endTexcoord.r;
    bFlipTriangle = ShouldFlipTriangle(data.start, data.end, topmiddlel, topleft);
    EmitQuad(bottommiddlert, bottomrightt, toprightt, topmiddlert, bottommiddlerws, bottomrightws, toprightws, topmiddlerws, data.startNormal, data.endNormal, texcoordmid1, texcoordmid2, bFlipTriangle);
}

void GenerateSegment(SegmentData data)
{
    switch (data.shape)
    {
        case 0: { GenerateSingleQuad(data); break; }
        case 1: { GenerateDoubleQuad(data); break; }
        case 2: { GenerateTripleQuad(data); break; }
    }
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

    SegmentData segment;
    segment.shape = controlpoint[0].shape; // all divisions except the last have the same shape as the first control point
    segment.start = start;
    segment.startWidthVector = startWidthVector;
    segment.startCurvatureHeight = controlpoint[0].thickness;
    segment.startNormal = controlpoint[0].normal;
    segment.startTexcoord = controlpoint[0].texcoord;

    if (controlpoint[0].subdivisions > 0)
    {
        float timestep = 1.0f/controlpoint[0].subdivisions;
        for (int i=1; i<controlpoint[0].subdivisions; i++)
        {
            // Endpoints
            float t = i*timestep;
            segment.end = bezier(bcp1, bcp2, bcp3, bcp4, t);
            segment.endWidthVector = mix(startWidthVector, endWidthVector, t);
            segment.endCurvatureHeight = mix(controlpoint[0].thickness, controlpoint[1].thickness, t);
            segment.endNormal = normalize(mix(controlpoint[0].normal, controlpoint[1].normal, t));
            segment.endTexcoord = mix(controlpoint[0].texcoord, controlpoint[1].texcoord, t);

            // Generate
            GenerateSegment(segment);

            // Update startpoints for next iteration
            segment.start = segment.end;
            segment.startWidthVector = segment.endWidthVector;
            segment.startCurvatureHeight = segment.endCurvatureHeight;
            segment.startNormal = segment.endNormal;
            segment.startTexcoord = segment.endTexcoord;
        }
    }

    // Generate last segment
    segment.shape = controlpoint[1].shape; // todo: solve transition
    segment.end = end;
    segment.endWidthVector = endWidthVector;
    segment.endCurvatureHeight = controlpoint[1].thickness;
    segment.endNormal = controlpoint[1].normal;
    segment.endTexcoord = controlpoint[1].texcoord;
    GenerateSegment(segment);
}