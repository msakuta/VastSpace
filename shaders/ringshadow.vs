// ringshadow.vs

varying vec3 pos;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	gl_TexCoord[2] = gl_MultiTexCoord2;
	pos = vec3(gl_Vertex);
	gl_Position = ftransform();
}
