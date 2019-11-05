#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vPosition[];
in vec3 vNormal[];
in vec4 vColor[];
in vec4 vTCoord[];

out vec3 gvPosition;
out vec3 gvNormal;
out vec4 gvColor;
out vec4 gvTCoord;

void main()
{
    // Flat face shading
    vec3 avgNormal = vec3(0.0);
    for (int i=0; i<gl_in.length(); i++)
    {
        avgNormal += vNormal[i];
    }
    avgNormal /= gl_in.length();

    // Update outputs to fragment shader
    for (int i=0; i<gl_in.length(); i++)
    {
        gl_Position = gl_in[i].gl_Position;
        gvPosition = vPosition[i];
        gvNormal = avgNormal; //vNormal[i];
        gvColor = vColor[i];
        gvTCoord = vTCoord[i];
        EmitVertex();
    }
    EndPrimitive();
}