#version 330 core
in vec3 wpos;
in vec3 fragNor;
in vec3 color;
out vec4 colorO;

uniform float NColor;
uniform float ShadingOp;
uniform vec3 specular;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float shine;

void main()
{
   vec3 normal = normalize(fragNor);
   if (NColor == 1.0f) {
      vec3 Ncolor = 0.5*normal + 0.5;
      colorO = vec4(Ncolor, 1.0);
   } else {
      vec3 light = normalize(lightPos); // vec3(1, 1, 1);
      float NdL = max(dot(normalize(fragNor), normalize(lightPos)), 0);
      vec3 view = -1 * wpos;
      vec3 reflection = -lightPos + 2 * NdL * normalize(fragNor);
      float VdR = max(dot(normalize(view), normalize(reflection)), 0);
      colorO = vec4(MatAmb*lightColor
            + ((MatDif * NdL) * lightColor)
            + (specular*pow(VdR, shine) * lightColor), 1.0);
   }
}
