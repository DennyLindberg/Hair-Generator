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
uniform vec3 unifiedNormalsCapsuleStart = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 unifiedNormalsCapsuleEnd = vec3(0.0f, 15.0f, 0.0f);
uniform float normalBlend = 0.9f;

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
    vec3 position_ws;
    vec3 normal_ws;
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


/*
    Used to compute normals for all vertices so that the hair gets a smoother appearance.
*/
vec3 GetUnifiedNormalLocalSpace(vec3 point_ls, vec3 defaultnormal)
{
    vec3 u = unifiedNormalsCapsuleEnd - unifiedNormalsCapsuleStart;
    vec3 v = point_ls - unifiedNormalsCapsuleStart;

    // Determine if the point_ls is outside the line segment
    float w_scalar = dot(v, normalize(u));
    if (w_scalar < 0.0f)
    {
        return normalize(mix(defaultnormal, normalize(point_ls-unifiedNormalsCapsuleStart), normalBlend));
    }
    else if (w_scalar > length(u))
    {
        return normalize(mix(defaultnormal, normalize(point_ls-unifiedNormalsCapsuleEnd), normalBlend));
    }
    else
    {
        vec3 perpendicular = normalize(u)*w_scalar;
        return normalize(mix(defaultnormal, normalize(v-perpendicular), normalBlend));
    }

    return defaultnormal;
}

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

void EmitPointData(PointData p)
{
    gl_Position = p.position;   // projection space
    vertex.normal_ws = p.normal;   // world space
    vertex.position_ws = p.position_ws.xyz; // world space
    vertex.color = p.position_ws; // world space
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

    // texcoord.rgb = (r=ustart, g=v, b=uend)
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

    // Compute normals based on local space coordinates, then transform them to world space
    p1.normal = (model * vec4(GetUnifiedNormalLocalSpace(p1.position.xyz, data.startNormal), 0.0f)).xyz;
    p2.normal = (model * vec4(GetUnifiedNormalLocalSpace(p2.position.xyz, data.startNormal), 0.0f)).xyz;
    p3.normal = (model * vec4(GetUnifiedNormalLocalSpace(p3.position.xyz, data.endNormal), 0.0f)).xyz;
    p4.normal = (model * vec4(GetUnifiedNormalLocalSpace(p4.position.xyz, data.endNormal), 0.0f)).xyz;

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

    // texcoord.rgb = (r=ustart, g=v, b=uend)
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

    // Compute normals based on local space coordinates, then transform them to world space
    p1.normal = (model * vec4(GetUnifiedNormalLocalSpace(p1.position.xyz, data.startNormal), 0.0f)).xyz;
    p2.normal = (model * vec4(GetUnifiedNormalLocalSpace(p2.position.xyz, data.startNormal), 0.0f)).xyz;
    p3.normal = (model * vec4(GetUnifiedNormalLocalSpace(p3.position.xyz, data.startNormal), 0.0f)).xyz;
    p4.normal = (model * vec4(GetUnifiedNormalLocalSpace(p4.position.xyz, data.endNormal), 0.0f)).xyz;
    p5.normal = (model * vec4(GetUnifiedNormalLocalSpace(p5.position.xyz, data.endNormal), 0.0f)).xyz;
    p6.normal = (model * vec4(GetUnifiedNormalLocalSpace(p6.position.xyz, data.endNormal), 0.0f)).xyz;
    
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
    /*
        p8 ----------- p7 ----end---- p6 ---------- p5
                      / |            /|            /|
                   /    |          /  |          /  |
       0.0      /      0.25     /    0.75     /     1.0  (u coordinate)
             /          |    /        |    /        |
          /             | /           | /           |
        p1 ----------- p2 ---start--- p3 ---------- p4
    */

    PointData p1;
    PointData p2;
    PointData p3;
    PointData p4;
    PointData p5;
    PointData p6;
    PointData p7;
    PointData p8;

    p1.normal = data.startNormal;
    p2.normal = data.startNormal;
    p3.normal = data.startNormal;
    p4.normal = data.startNormal;
    p5.normal = data.endNormal;
    p6.normal = data.endNormal;
    p7.normal = data.endNormal;
    p8.normal = data.endNormal;

    // texcoord.rgb = (r=ustart, g=v, b=uend)
    // Set corners and mix later
    p1.texcoord = vec2(data.startTexcoord.r, data.startTexcoord.g);
    p4.texcoord = vec2(data.startTexcoord.b, data.startTexcoord.g);
    p8.texcoord = vec2(data.endTexcoord.r, data.endTexcoord.g);
    p5.texcoord = vec2(data.endTexcoord.b, data.endTexcoord.g);
    // Mix inbetweens
    p2.texcoord = mix(p1.texcoord, p4.texcoord, 0.25f);
    p3.texcoord = mix(p1.texcoord, p4.texcoord, 0.75f);
    p7.texcoord = mix(p8.texcoord, p5.texcoord, 0.25f);
    p6.texcoord = mix(p8.texcoord, p5.texcoord, 0.75f);

    // Localspace
    // Set corners and mix later (same as with UVs)
    p1.position = vec4(data.start + data.startWidthVector, 1.0f);
    p4.position = vec4(data.start - data.startWidthVector, 1.0f);
    p8.position = vec4(data.end + data.endWidthVector, 1.0f) ;
    p5.position = vec4(data.end - data.endWidthVector, 1.0f);
    // Mix inbetweens
    p2.position = mix(p1.position, p4.position, 0.25f);
    p3.position = mix(p1.position, p4.position, 0.75f);
    p7.position = mix(p8.position, p5.position, 0.25f);
    p6.position = mix(p8.position, p5.position, 0.75f);
    bool bFlipTriangle1 = ShouldFlipTriangle(((p1.position+p2.position)/2.0).xyz, ((p8.position+p7.position)/2.0).xyz, p7.position.xyz, p8.position.xyz);
    bool bFlipTriangle2 = ShouldFlipTriangle(data.start, data.end, p6.position.xyz, p7.position.xyz);
    bool bFlipTriangle3 = ShouldFlipTriangle(((p3.position+p4.position)/2.0).xyz, ((p6.position+p5.position)/2.0).xyz, p5.position.xyz, p6.position.xyz);

    // Apply thickness
    vec4 startThicknessOffset = vec4(data.startNormal * data.startCurvatureHeight / 2.0f, 0.0f);
    vec4 endThicknessOffset = vec4(data.endNormal * data.endCurvatureHeight / 2.0f, 0.0f);
    p1.position -= startThicknessOffset;
    p2.position += startThicknessOffset;
    p3.position += startThicknessOffset;
    p4.position -= startThicknessOffset;
    p5.position -= endThicknessOffset;
    p6.position += endThicknessOffset;
    p7.position += endThicknessOffset;
    p8.position -= endThicknessOffset;
    
    // Compute normals based on local space coordinates, then transform them to world space
    p1.normal = (model * vec4(GetUnifiedNormalLocalSpace(p1.position.xyz, data.startNormal), 0.0f)).xyz;
    p2.normal = (model * vec4(GetUnifiedNormalLocalSpace(p2.position.xyz, data.startNormal), 0.0f)).xyz;
    p3.normal = (model * vec4(GetUnifiedNormalLocalSpace(p3.position.xyz, data.startNormal), 0.0f)).xyz;
    p4.normal = (model * vec4(GetUnifiedNormalLocalSpace(p4.position.xyz, data.startNormal), 0.0f)).xyz;
    p5.normal = (model * vec4(GetUnifiedNormalLocalSpace(p5.position.xyz, data.endNormal), 0.0f)).xyz;
    p6.normal = (model * vec4(GetUnifiedNormalLocalSpace(p6.position.xyz, data.endNormal), 0.0f)).xyz;
    p7.normal = (model * vec4(GetUnifiedNormalLocalSpace(p7.position.xyz, data.endNormal), 0.0f)).xyz;
    p8.normal = (model * vec4(GetUnifiedNormalLocalSpace(p8.position.xyz, data.endNormal), 0.0f)).xyz;

    // Compute world space
    // width vector points "left" to p1 and p4
    p1.position_ws = model * p1.position;
    p2.position_ws = model * p2.position;
    p3.position_ws = model * p3.position;
    p4.position_ws = model * p4.position;
    p5.position_ws = model * p5.position;
    p6.position_ws = model * p6.position;
    p7.position_ws = model * p7.position;
    p8.position_ws = model * p8.position;

    // Project into clip space
    mat4 vp = projection * view;
    p1.position = vp * p1.position_ws;
    p2.position = vp * p2.position_ws;
    p3.position = vp * p3.position_ws;
    p4.position = vp * p4.position_ws;
    p5.position = vp * p5.position_ws;
    p6.position = vp * p6.position_ws;
    p7.position = vp * p7.position_ws;
    p8.position = vp * p8.position_ws;
    
    if (bFlipTriangle1)
    {
        EmitTriangle(p1, p2, p8);
        EmitTriangle(p8, p2, p7);
    }
    else
    {
        EmitTriangle(p1, p2, p7);
        EmitTriangle(p7, p8, p1);
    }
        
    if (bFlipTriangle2)
    {
        EmitTriangle(p2, p3, p7);
        EmitTriangle(p7, p3, p6);
    }
    else
    {
        EmitTriangle(p2, p3, p6);
        EmitTriangle(p6, p7, p2);
    }

    if (bFlipTriangle3)
    {
        EmitTriangle(p3, p4, p6);
        EmitTriangle(p6, p4, p5);
    }
    else
    {
        EmitTriangle(p3, p4, p5);
        EmitTriangle(p5, p6, p3);
    }
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