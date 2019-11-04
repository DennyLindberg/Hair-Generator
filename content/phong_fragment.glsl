#version 330

layout(location = 0) out vec4 color;

uniform sampler2D textureSampler;
uniform vec4 lightColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

in vec3 vPosition;
in vec3 vNormal;
in vec4 vColor;
in vec4 vTCoord;

void main() 
{
    vec3 lightDir = normalize(lightPosition-vPosition);
    vec3 camDir = normalize(cameraPosition-vPosition);
    vec3 normal = normalize(vNormal);

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

    vec3 texSample = texture(textureSampler, vTCoord.rg).rgb; // ignore alpha
    color = totalLightContribution * vec4(texSample, 1.0f);
}