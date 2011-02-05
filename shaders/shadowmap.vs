varying vec3 view;
//varying vec3 nrm;
varying float diffuse;
//varying vec4 col;

void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);
//	col = gl_Color;

//	nrm = gl_NormalMatrix * gl_Normal;
	vec3 fnormal = normalize(gl_NormalMatrix * gl_Normal);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	diffuse = max(0., dot(flight, fnormal));

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[2] = gl_TextureMatrix[2] * gl_ModelViewMatrix * gl_Vertex;

	gl_Position = ftransform();
}
