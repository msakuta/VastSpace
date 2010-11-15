varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);
	col = gl_Color;

	nrm = /*gl_NormalMatrix **/ gl_Normal;

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;
	gl_TexCoord[3] = gl_TextureMatrix[3] * gl_MultiTexCoord3;

	gl_Position = ftransform();

	float phi = atan(nrm[2], nrm[0]);
	texa0[0] = -sin(phi);
	texa0[1] = 0.;
	texa0[2] = cos(phi);
	texa1 = cross(normalize(nrm), texa0);
}
