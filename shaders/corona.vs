varying vec4 col;

void main(void)
{
	col = gl_Color;
	gl_Position = ftransform();

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
}
