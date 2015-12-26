varying vec3 view;
varying vec3 nrm;

#include "shaders/shadowmap.vs"


void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);

	nrm = gl_NormalMatrix * gl_Normal;

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	gl_Position = ftransform();

	shadowMapVertex();
}
