uniform samplerCube texture;
uniform int lightCount;
#include "shaders/tonemap.fs"
#include "shaders/shadowmap.fs"
uniform bool shadowCast;
uniform sampler3D noise3D;

varying vec3 view;
varying vec3 nrm;
varying vec3 pos;

// Detail texture thresholds in meters, could be uniforms
const float detailFadeEnd = 1000.;
const float detailFadeStart = 500.;

void main (void)
{
	vec3 normal = normalize(nrm);
	vec3 fnormal = normalize(normal);
	vec3 fview = normalize(view);
	vec3 diffuse = vec3(0,0,0);
	for(int i = 0; i < lightCount; i++){
		vec3 flight = normalize(gl_LightSource[i].position.xyz);
		float fdiffuse = dot(flight, fnormal);
		float shadow = shadowCast ? shadowMapIntensity(1. / fdiffuse) : 1.;
		vec3 ldiffuse = max(vec3(0,0,0), fdiffuse * gl_LightSource[i].diffuse.xyz);

		if(i == 0)
			diffuse += (shadow * .8 + .2) * ldiffuse;
		else
			diffuse += ldiffuse;
	}
	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));

	// Add noise as a detail texture if the fragment is close to the camera.
	float z = -view.z;
	if(z < detailFadeEnd){
		// Detail texture fades out linearly.
		// Other decay functions are possible, but it's robust and responds well.
		// The important thing is that farther pixels won't even need to sample
		// this texture.
		float f = max(0, (z - detailFadeStart) / (detailFadeEnd - detailFadeStart));
		vec3 noiseInput = pos;
		texColor *= 1. + (0.5 * (1. - f)) * (texture3D(noise3D, 1.e5 * noiseInput)[0] - .5);
	}

	texColor.xyz *= diffuse + 0.1;
	texColor.xyz += gl_FrontLightProduct[0].ambient.xyz;
	texColor[3] = 1.;
	gl_FragColor = toneMapping(texColor);

	// Debug drawing the fading borders
//	if(detailFadeEnd < z && z < detailFadeEnd * 1.01)
//		gl_FragColor.xyz = 1.;
//	if(detailFadeStart * 0.99 < z && z < detailFadeStart)
//		gl_FragColor.xyz = vec3(1,0,0);
}
