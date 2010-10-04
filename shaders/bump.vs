varying vec3 view;
varying vec3 nrm;
varying vec4 col;

void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);
	col = gl_Color;

	nrm = gl_NormalMatrix * gl_Normal;

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	gl_Position = ftransform();
}
