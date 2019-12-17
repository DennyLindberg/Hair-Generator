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
uniform bool bDrawDebugNormals = false;
uniform vec3 darkColor = vec3(33.0f/255.0f, 17.0f/255.0f, 4.0f/255.0f);
uniform vec3 lightColor = vec3(145.0f/255.0f, 123.0f/255.0f, 104.0f/255.0f)*0.7;
uniform float maskCutoff = 0.25f;

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
    vec3 position_ws;
    vec3 normal_ws;
    vec4 color;
    vec4 tcoord;
} fragment;

// Light is computed in World Space
vec4 PhongLight()
{
    // Light computation
    vec3 lightDir = normalize(light_position-fragment.position_ws);
    vec3 camDir = normalize(camera_position-fragment.position_ws);
    vec3 normal = normalize(fragment.normal_ws);

    // Give ambient regions some depth based on camera direction (otherwise the non-lit regions become flat)
    float cameraContrib = clamp(dot(normal, camDir), 0.0, 1.0);
    vec3 ambientLight = cameraContrib * vec3(0.2);

    // Ordinary phong diffuse model
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
    if (alphaSample.r < maskCutoff)
    {
        discard;
    }
    
    float colorBlend = bRenderHairFlat? 0.5f : texture(colorSampler, texCoord).r;
    vec4 colorSample = vec4(mix(darkColor, lightColor, colorBlend), 1.0f);
    vec4 idSample = texture(idSampler, texCoord);

    if (bDrawDebugNormals)
    {
        color = vec4(normalize(fragment.normal_ws), 1.0f);
    }
    else
    {
        color = PhongLight() * colorSample;
    }
}