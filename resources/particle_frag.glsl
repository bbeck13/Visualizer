#version 330 core
uniform sampler2D alphaTexture;
in vec4 partCol;
out vec4 Outcolor;

void main()
{
	float alpha = texture(alphaTexture, gl_PointCoord).r;
	Outcolor = vec4(partCol.rgb, partCol.a*alpha);
}
