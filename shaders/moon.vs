varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);
	col = gl_Color;

	nrm = gl_NormalMatrix * gl_Normal;

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	gl_Position = ftransform();

	float theta = atan(gl_TexCoord[0][1], sqrt(gl_TexCoord[0][0] * gl_TexCoord[0][0] + gl_TexCoord[0][2] * gl_TexCoord[0][2]));
//	float phi = atan(gl_TexCoord[0][0], gl_TexCoord[0][2]);
	float phi = atan(nrm[2], nrm[0]);
	texa0[0] = -sin(phi);
	texa0[1] = 0.;
	texa0[2] = cos(phi);
/*	texa1[0] = sin(theta) * sin(phi);
	texa1[1] = cos(theta);
	texa1[2] = sin(theta) * cos(phi);*/
	texa1 = cross(normalize(nrm), texa0);
}
