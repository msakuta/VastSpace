uniform samplerCube texture;
uniform int lightCount;
#include "shaders/tonemap.fs"

varying vec3 view;
varying vec3 nrm;

void main (void)
{
	vec3 normal = normalize(nrm);
	vec3 fnormal = normalize(normal);
	vec3 fview = normalize(view);
	vec3 diffuse = vec3(0,0,0);
	for(int i = 0; i < lightCount; i++){
		vec3 flight = normalize(gl_LightSource[i].position.xyz);

		diffuse += max(vec3(0,0,0), dot(flight, fnormal) * gl_LightSource[i].diffuse.xyz);
	}
	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));
	texColor.xyz *= diffuse + 0.1;
	texColor.xyz += gl_FrontLightProduct[0].ambient.xyz;
	texColor[3] = 1.;
	gl_FragColor = toneMapping(texColor);
}
