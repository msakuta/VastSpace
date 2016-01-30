uniform samplerCube texture;
uniform samplerCube bumptexture;
uniform int lightCount;
uniform mat3 rotation;
#  include "shaders/tonemap.fs"
#include "shaders/shadowmap.fs"
uniform sampler3D noise3D;
uniform bool shadowCast;

varying vec3 view;
varying vec3 nrm;
varying vec3 pos;

// Detail texture thresholds in meters, could be uniforms
const float detailFadeEnd = 1000.;
const float detailFadeStart = 500.;

// Detail texture thresholds in meters, could be uniforms
const float bumpFadeEnd = 50000.;
const float bumpFadeStart = 25000.;

void main (void)
{
	vec3 texsample = textureCube(bumptexture, gl_TexCoord[0].xyz).xyz;
	vec3 texnorm0 = texsample - vec3(0.5, .5, .5);
	vec3 fnormal = normalize(rotation * texnorm0);
	vec3 fnormalV = normalize(nrm);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 diffuse = vec3(0,0,0);
	vec3 diffuseV = vec3(0,0,0);
	for(int i = 0; i < lightCount; i++){
		vec3 flight = normalize(gl_LightSource[i].position.xyz);
		float fdiffuse = dot(flight, fnormal);
		float fdiffuseV = dot(flight, fnormalV);
		float shadow = shadowCast ? shadowMapIntensity(1. / fdiffuseV) : 1.;
		vec3 ldiffuse = max(vec3(0,0,0), fdiffuseV * gl_LightSource[i].diffuse.xyz);


		if(i == 0){
			diffuse += (shadow * .8 + .2) * max(vec3(0,0,0), fdiffuse * gl_LightSource[i].diffuse.xyz);
			diffuseV += (shadow * .8 + .2) * ldiffuse;
		}
		else{
			diffuse += max(vec3(0,0,0), fdiffuse * gl_LightSource[i].diffuse.xyz);
			diffuseV += ldiffuse;
		}
	}

	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));

	// Add noise as a detail texture if the fragment is close to the camera.
	float z = -view.z;
	if(z < detailFadeEnd){
		// Detail texture fades out linearly.
		// Other decay functions are possible, but it's robust and responds well.
		// The important thing is that farther pixels won't even need to sample
		// this texture.
		vec3 noiseInput = pos;
		float f = max(0, min(1, (z - detailFadeStart) / (detailFadeEnd - detailFadeStart)));
		texColor *= 1. + (0.5 * (1. - f)) * (texture3D(noise3D, 1.e5 * noiseInput)[0] - .5);
	}

	float bumpFactor = max(0, min(1, (z - bumpFadeStart) / (bumpFadeEnd - bumpFadeStart)));

	texColor.xyz *= diffuse * bumpFactor + diffuseV * (1. - bumpFactor)/* + ambient*/;
	texColor[3] = 1.;
	gl_FragColor = toneMapping(texColor);

	// Debug outputs
//	gl_FragColor.xyz = gl_TexCoord[0].xyz;
//	gl_FragColor.xyz = fnormal;
//	gl_FragColor.xyz = texsample1;
//	gl_FragColor.xyz = vec3(textureCube(bumptexture, gl_TexCoord[0].xyz));
}
