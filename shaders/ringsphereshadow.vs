// ringsphereshadow.vs

varying vec3 norm;

void main(void)
{
	// Standard varying coordinates.
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	gl_Position = ftransform();

	// Interpolate normal vector to implement Phong shading.
	norm = gl_NormalMatrix * gl_Normal;
}
