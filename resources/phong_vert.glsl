#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec3 vertTex;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

uniform mat4 P;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float NColor;
uniform float ShadingOp;
uniform vec3 MatAmb;
uniform vec3 MatDif;
out vec3 fragNor;
out vec3 color;
out vec3 wpos;
uniform vec3 specular;
uniform float shine;

void main()
{
   vec4 vpos = ModelMatrix * vertPos;
   vpos = ViewMatrix * vpos;
   gl_Position = P * vpos;
   fragNor = ((ModelMatrix) * vec4(vertNor, 0.0)).xyz;
   wpos = ((ModelMatrix) * vertPos).xyz;
   color = vec3(0.0, 0.0, 0.0);
}
