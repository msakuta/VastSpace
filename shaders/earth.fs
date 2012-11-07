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
vec4 ocean(vec3 v){
	// height and noise3D uniform variables are defined in earth_cloud_noise.fs.
	float f2 = 1./*min(1., 20. / height)*/;
	v *= 50 * 16.;

	// Rotated input vector for combination to enhance period of the noise.
	// The period of cells formed by v and v2 vectors is desired to be
	// as large as possible.
	vec3 v2 = vec3(v.x * cos(1) + v.y * sin(1), v.x * -sin(1) + v.y * cos(1), v.z);

	// Accumulate multiple noises to reduce artifacts caused by finite texture size.
	return 0.2 * (
		(1.0 * f2 * (texture3D(noise3D, 128. * v) - vec4(.5)) + // Fine noise
		1.0 * f2 * (texture3D(noise3D, 128. * v2) - vec4(.5))) / (1. - 2. * view.z) + // Fine rotated noise
		(texture3D(noise3D, 32. * v) - vec4(0.5)) / (1. - 1. * view.z) + // High octave noise
		(texture3D(noise3D, 8. * v) - vec4(0.5)) / (1. - 0.2 * view.z) // Very high octave noise
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
		specular = 0.7, shininess = 50., waving = true;
	else
		specular = 0.25, shininess = 5., waving = false;
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
		noiseInput += 2e-6 * noisePos;
	vec4 noise = ocean(noiseInput);

	vec3 texnorm = (texa0 * (texnorm0[2]) + texa1 * (texnorm0[1]) + noise.xyz);
	vec3 fnormal = normalize((normal) + (texnorm) * 1.);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal) + .0);

	// Dot product of light and geometry normal.
	float dtn = dot(flight, normal);
	// The ambient factor is calculated with the same formula as the atmosphere's.
	float ambf = 0. < dtn ? 1. : pow(1. + dtn, 8.)/* max(0., min(1., (dtn + 0.1) / 0.2))*/;
	// Obtain cloud thickness above. TODO: project sun's ray of light onto sphere.
	float cloud = cloudfunc(cloudtexture, vec3(gl_TexCoord[2]), view.z).a;
	// How diffuse light is blocked by cloud. The coefficient of cloud looks better to be less than 1.
	float cloudBlock = 1. - 0.75 * cloud;
	// Diffuse strength should be scaled with ambf too, or subtle artifacts appear in sunrise and sunset.
	diffuse *= ambf * cloudBlock;
	// Specular reflection won't reach the night side of the Earth.
	specular *= ambf * cloudBlock;
	// If you're far from the ocean surface, small waves are averaged and apparent shininess decreases.
	float innerShininess = shininess * 50.;
	innerShininess /= min(50., 1. - 0.2 * view.z);

	float ambient = 0.002 + 0.15 * ambf;
	ambient *= 1. - 0.5 * cloud;
	texColor *= (diffuse + ambient) * (1. + 2. * noise.w);
	texColor += specular * vshininess * pow(shininess * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);
	// Another specular for sun's direct light
	texColor += specular * vshininess * pow(innerShininess * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);

	if(sundot < 0.1)
		texColor += textureCube(lightstexture, vec3(gl_TexCoord[0])) * min(.02, 5. * (-sundot + 0.1));
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
	
	gl_FragColor = toneMapping(texColor);
}
