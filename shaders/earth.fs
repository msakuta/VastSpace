#include "shaders/tonemap.fs"
#include "shaders/earth_cloud_noise.fs"

uniform samplerCube texture;
uniform mat3 invEyeRot3x3;
uniform samplerCube bumptexture;
uniform samplerCube cloudtexture;
uniform samplerCube lightstexture;
uniform float time;
uniform sampler1D tex1d;
uniform float ringmin, ringmax;
uniform vec3 ringnorm;
uniform vec3 noisePos;

varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

// This will bluescreen in Radeon HD 6870.
//#extension GL_EXT_gpu_shader4 : enable

bool waving;

/// Returns ocean wave noise pattern
vec3 ocean(vec3 v){
	// height and noise3D uniform variables are defined in earth_cloud_noise.fs.
	float f2 = min(1., 20. / height);
	v *= 50;

	// Rotated input vector for combination to enhance period of the noise.
	// The period of cells formed by v and v2 vectors is desired to be
	// as large as possible.
	vec3 v2 = vec3(v.x * cos(1) + v.y * sin(1), v.x * -sin(1) + v.y * cos(1), v.z);

	// Accumulate multiple noises to reduce artifacts caused by finite texture size.
	return 0.1 * vec3(
		2.0 * f2 * (texture3D(noise3D, 128. * v) - vec4(.5)) + // Fine noise
		2.0 * f2 * (texture3D(noise3D, 128. * v2) - vec4(.5)) + // Fine rotated noise
		(texture3D(noise3D, 32. * v) - vec4(0.5)) + // High octave noise
		(texture3D(noise3D, 8. * v) - vec4(0.5)) // Very high octave noise
		);
}

void main (void)
{
	vec3 normal = normalize(nrm);
	vec3 flight = (gl_LightSource[0].position.xyz);
	vec3 texCoord = vec3(gl_TexCoord[0]);

	vec4 texColor = textureCube(texture, texCoord);
	float specular;
	float shininess;
	if(texColor[2] > texColor[0] + texColor[1])
		specular = 1., shininess = 20., waving = true;
	else
		specular = .3, shininess = 5., waving = false;
	float sundot = dot(flight, normal);
	float dawness = sundot * 8.;
	specular *= max(.1, min(1., dawness));
	dawness = 1. - exp(-dawness * dawness);
	vec4 vshininess = vec4(.75, .2 + .6 * dawness, .3 + .7 * dawness, 0.);

	vec3 texnorm0 = vec3(textureCube(bumptexture, texCoord)) - vec3(1., .5, .5);
	texnorm0[1] *= 5.;
	texnorm0[2] *= 5.;

	// Obtain ocean noise to add to normal vector.
	// The noise position moves over time, so it would be a random source.
	// But do not add in case of rendering land.
	vec3 noiseInput = texCoord;
	if(waving)
		noiseInput += 1e-5 * noisePos;
	vec3 noise = ocean(noiseInput);

	vec3 texnorm = (texa0 * (texnorm0[2]) + texa1 * (texnorm0[1]) + noise);
	vec3 fnormal = normalize((normal) + (texnorm) * 1.);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal) + .1);

	float ambient = 0.001;
	texColor *= diffuse + ambient;
	texColor += specular * vshininess * pow(shininess * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);

	texColor *= 1. - max(0., .5 * float(cloudfunc(cloudtexture, vec3(gl_TexCoord[2]), view.z)));
	if(sundot < 0.1)
		texColor += textureCube(lightstexture, vec3(gl_TexCoord[0])) * min(.02, 5. * (-sundot + 0.1));
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
	
	gl_FragColor = toneMapping(texColor);
}
