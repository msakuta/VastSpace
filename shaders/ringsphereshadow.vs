// ringshadow.vs

attribute vec3 apos;
varying vec3 pos;

void main(void)
{
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	pos = vec3(apos);
	gl_Position = ftransform();
}
