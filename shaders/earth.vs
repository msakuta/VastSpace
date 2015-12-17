varying vec3 view;
varying vec3 nrm;
varying vec4 col;

#include "shaders/shadowmap.vs"

void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);
	col = gl_Color;

	nrm = gl_NormalMatrix * gl_Normal;

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;
	gl_TexCoord[3] = gl_TextureMatrix[3] * gl_MultiTexCoord3;

	gl_Position = ftransform();

	shadowMapVertex();
}
