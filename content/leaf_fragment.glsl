#version 330

layout(location = 0) out vec4 color;

uniform sampler2D textureSampler;
uniform float sssBacksideAmount;
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

    // Ordinary phong diffuse model with fake SSS
    float angleContribution = dot(normal, lightDir);
    float directLightDot = clamp(angleContribution, 0.0, 1.0);
    float sssLightDot = abs(angleContribution);
    float backSideFactor = abs(clamp(angleContribution, -1.0f, 0.0f));
    float directLightContribution = mix(directLightDot, 0.75 + 0.25*sssLightDot, sssBacksideAmount);
    float lightStrength = lightColor.a;
    vec3 diffuseLight = lightStrength * directLightContribution * lightColor.rgb;
    
    vec3 ambientLight = vec3(0.2);
    vec3 specularLight = vec3(0.0); // Specular not yet implemented
    vec4 totalLightContribution = vec4(ambientLight + diffuseLight + specularLight, 1.0);

    vec4 texSample = texture(textureSampler, vTCoord.rg);
    vec4 surfaceColorFront = mix(vec4(0.9f, 0.8f, 0.2f, 1.0f), vec4(0.7f, 0.5f, 0.2f, 1.0f), texSample.a);
    vec4 surfaceColorBack = mix(vec4(0.4f, 0.3f, 0.4f, 1.0f), vec4(0.05f, 0.1f, 0.05f, 1.0f), texSample.a);
    vec4 surfaceColor = mix(surfaceColorFront, surfaceColorBack, backSideFactor);
    color = totalLightContribution * vec4(surfaceColor.rgb, 1.0f);
}
