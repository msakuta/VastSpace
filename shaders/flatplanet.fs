uniform samplerCube texture;
#include "shaders/tonemap.fs"

varying vec3 view;
varying vec3 nrm;

void main (void)
{
	vec3 normal = normalize(nrm);
	vec3 fnormal = normalize(normal);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal));

	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));
	texColor *= diffuse;
	texColor[3] = 1.;
	gl_FragColor = toneMapping(texColor);
}
