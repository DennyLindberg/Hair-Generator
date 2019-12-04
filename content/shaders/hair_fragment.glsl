#version 420 core

layout(location = 0) out vec4 color;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;       // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;             // 64 Column1, 80 Column2, 96 Column3, 112 Column4
    vec3 camera_position;  // 128
};
uniform mat4 model;
uniform bool bRenderHairFlat = false;
uniform vec3 unifiedNormalsCapsuleStart = vec3(0.0f, 0.0f, 0.05f);
uniform vec3 unifiedNormalsCapsuleEnd = vec3(0.0f, 0.3f, 0.05f);
uniform float normalBlend = 0.9f;

layout (std140, binding = 2) uniform Light
{
    vec3 light_position;  // 0+16 (occupies 4N when alone)
    vec4 light_color;     // 16+16
};

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 1) uniform sampler2D alphaSampler;
layout(binding = 2) uniform sampler2D idSampler;

// World space attributes
in VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} fragment;

vec3 GetUnifiedNormal(vec3 point)
{
    vec3 start = unifiedNormalsCapsuleStart;
    vec3 end = unifiedNormalsCapsuleEnd;

    vec3 u = end - start;
    vec3 v = point - start;

    // Determine if the point is outside the line segment
    float w_scalar = dot(v, normalize(u));
    if (w_scalar < 0.0f)
    {
        return normalize(mix(fragment.normal, normalize(point-start), normalBlend));
    }
    else if (w_scalar > length(u))
    {
        return normalize(mix(fragment.normal, normalize(point-end), normalBlend));
    }
    else
    {
        vec3 perpendicular = normalize(u)*w_scalar;
        return normalize(mix(fragment.normal, normalize(v-perpendicular), normalBlend));
    }

    return fragment.normal;
}

vec4 PhongLight()
{
    // Light computation
    vec3 lightDir = normalize(light_position-fragment.position);
    vec3 camDir = normalize(camera_position-fragment.position);
    //vec3 normal = normalize(fragment.normal);
    //vec3 normal = normalize(fragment.position - unifiedNormalsOrigin);
    vec3 normal = GetUnifiedNormal(fragment.position);

    // Give ambient regions some depth
    float cameraContrib = clamp(dot(normal, camDir), 0.0, 1.0);
    vec3 ambientLight = cameraContrib * vec3(0.2);

    // Ordinary phong diffuse model with fake SSS
    float directLightDot = clamp(dot(normal, lightDir), 0.0, 1.0);
    float lightStrength = light_color.a;
    vec3 diffuseLight = lightStrength * directLightDot * light_color.rgb;

    // Specular not yet implemented
    vec3 specularLight = vec3(0.0);

    return vec4(ambientLight + diffuseLight + specularLight, 1.0);
}

void main()
{
    vec2 texCoord = fragment.tcoord.rg;

    // Masked discard
    vec4 alphaSample = texture(alphaSampler, texCoord);
    if (alphaSample.r < 0.25f)
    {
        discard;
    }
    
    vec3 baseColor = vec3(33.0f/255.0f, 17.0f/255.0f, 4.0f/255.0f);
    vec3 stripeColor = vec3(145.0f/255.0f, 123.0f/255.0f, 104.0f/255.0f)*0.7;

    float colorBlend = bRenderHairFlat? 0.5f : texture(colorSampler, texCoord).r;
    vec4 colorSample = vec4(mix(baseColor, stripeColor, colorBlend), 1.0f);
    vec4 idSample = texture(idSampler, texCoord);
    color = PhongLight() * colorSample;
    //vec3 normal = GetUnifiedNormal(fragment.position);
    //color = vec4(normal, 1.0f);
}