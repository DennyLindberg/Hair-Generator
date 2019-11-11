#version 420 core

layout(location = 0) out vec4 color;

layout (std140, binding = 1) uniform Camera
{
    mat4 projection;    // 0 Column1, 16 Column2, 32 Column3, 48 Column4
    mat4 view;          // 64 Column1, 80 Column2, 96 Column3, 112 Column4
};
uniform mat4 model;

uniform sampler2D textureSampler;
uniform vec4 lightColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

// World space attributes
in VertexAttrib
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec4 tcoord;
} fragment;

void main() 
{
    vec3 lightDir = normalize(lightPosition-fragment.position);
    vec3 camDir = normalize(cameraPosition-fragment.position);
    vec3 normal = normalize(fragment.normal);

    // Give ambient regions some depth
    float cameraContrib = clamp(dot(normal, camDir), 0.0, 1.0);
    vec3 ambientLight = cameraContrib * vec3(0.2);

    // Ordinary phong diffuse model with fake SSS
    float directLightDot = clamp(dot(normal, lightDir), 0.0, 1.0);
    float lightStrength = lightColor.a;
    vec3 diffuseLight = lightStrength * directLightDot * lightColor.rgb;

    // Specular not yet implemented
    vec3 specularLight = vec3(0.0);

    vec4 totalLightContribution = vec4(ambientLight + diffuseLight + specularLight, 1.0);

    //vec3 texSample = texture(textureSampler, gvTCoord.rg).rgb; // ignore alpha
    //color = totalLightContribution * vec4(texSample, 1.0f);

    color = totalLightContribution * vec4(0.5f, 0.5f, 0.5f, 1.0f);
    //color = fragment.color;
}