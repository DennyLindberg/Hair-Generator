#version 420 core

layout(location = 0) out vec4 color;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;       // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;             // 64 Column1, 80 Column2, 96 Column3, 112 Column4
    vec3 camera_position;  // 128
};
uniform mat4 model;

layout (std140, binding = 2) uniform Light
{
    vec3 light_position;  // 0+16 (occupies 4N when alone)
    vec4 light_color;     // 16+16
};

layout(binding = 0) uniform sampler2D shellColorSampler;
layout(binding = 1) uniform sampler2D shellAlphaSampler;
layout(binding = 2) uniform sampler2D scalpSampler;

// World space attributes
in VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} fragment;

vec4 PhongLight()
{
    // Light computation
    vec3 lightDir = normalize(light_position-fragment.position);
    vec3 camDir = normalize(camera_position-fragment.position);
    vec3 normal = normalize(fragment.normal);

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
    vec2 texCoord = fragment.tcoord.rg * 200.0f;

    // select EITHER the head texture or the shell texture
    vec4 light = PhongLight();
    vec3 headcolor = vec3(0.5f, 0.5f, 0.5f);
    if (fragment.color.r == 0.0f)
    {
        color = light * vec4(headcolor, 1.0f);
    }
    else
    {
        vec4 shellSampleAlpha = texture(scalpSampler, texCoord*0.1f);

        // Masked discard
        if (shellSampleAlpha.r > 0.98f)
        {
            discard;
        }

        //float alpha = smoothstep(0.0f, 0.25f, fragment.color.r); // remap [0, 1.0] to [0, 0.25]
        //vec3 blend = mix(headcolor, texture(shellColorSampler, texCoord).xyz, alpha);
        //color = light * vec4(blend, 1.0f);
        color = vec4(vec3(1.0f-shellSampleAlpha.r), 1.0f);
    }
}