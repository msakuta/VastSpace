varying vec3 view;
//varying vec3 nrm;
varying float diffuse[2];
//varying vec4 col;

void main(void)
{
	view = vec3(gl_ModelViewMatrix * gl_Vertex);
//	col = gl_Color;
	gl_Position = ftransform();

//	nrm = gl_NormalMatrix * gl_Normal;
	vec3 fnormal = normalize(gl_NormalMatrix * gl_Normal);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	diffuse[0] = max(0., dot(flight, fnormal));

	flight = (/*gl_NormalMatrix * */(gl_LightSource[1].position).xyz - (gl_ModelViewMatrix * gl_Vertex).xyz);
	float dist = length(flight);
	float attenuation = 1.0 / (gl_LightSource[1].constantAttenuation
		+ gl_LightSource[1].linearAttenuation * dist
		+ gl_LightSource[1].quadraticAttenuation * dist * dist);
	flight = normalize(flight);
	diffuse[1] = max(0., dot(flight, fnormal)) * attenuation;

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	gl_TexCoord[2] = gl_TextureMatrix[2] * gl_ModelViewMatrix * gl_Vertex;
	gl_TexCoord[3] = gl_TextureMatrix[3] * gl_ModelViewMatrix * gl_Vertex;
	gl_TexCoord[4] = gl_TextureMatrix[4] * gl_ModelViewMatrix * gl_Vertex;

}
