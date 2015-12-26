varying vec3 view;
varying vec3 nrm;
varying vec3 pos;

#include "shaders/shadowmap.vs"


void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);

	nrm = gl_NormalMatrix * gl_Normal;

	pos = normalize(gl_Vertex);

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	gl_Position = ftransform();

	shadowMapVertex();
}
