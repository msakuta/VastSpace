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

varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

// This will bluescreen in Radeon HD 6870.
//#extension GL_EXT_gpu_shader4 : enable

bool waving;

vec3 anoise1(vec3 v){
	return noise3(v);
}

vec3 anoise2(vec3 v){
	return 2. * anoise1(v * 16.) + noise3(v);
}

vec3 anoise3(vec3 v){
	return 1. * anoise2(v * 16.) + noise3(v);
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
	vec3 wavedir = vec3(1,1,0);
	vec3 wavedir2 = vec3(.25,.5,1);
	vec3 noise = .05 * vec3(anoise2(8. * 1000. * texCoord));
//		+ sin(16. * 8. * 1000. * dot(wavedir, texCoord) + time) * wavedir * .05
//		+ sin(16. * 8. * 1000. * dot(wavedir2, texCoord) + time) * wavedir2 * .05;
	vec3 texnorm = (texa0 * (texnorm0[2]) + texa1 * (texnorm0[1]) + noise);
	vec3 fnormal = normalize((normal) + (texnorm) * 1.);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal) + .1);

//	texColor = vec4((anoise3(vec3(gl_TexCoord[0]) * 1000.) + vec3(1,1,1))/2, 0);
	float ambient = 0.001;
	texColor *= diffuse + ambient;
	texColor += specular * vshininess * pow(shininess * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);


	// Aquire transformed vectors in eye space.
	if(false){
		// Lighting calculation
		vec3 npos = normalize(gl_NormalMatrix * normal);
		vec3 pos = /*gl_NormalMatrix **/ normalize(vec3(gl_TexCoord[3]));
		vec3 lnorm = /*gl_NormalMatrix **/ ringnorm;

		// Calculate projected point onto ring plane.
		float normp = dot(pos, lnorm);
		float lightp = dot(flight, lnorm);
		vec3 q = pos - normp / lightp * flight;

	//	float temp = 1. - abs(dot(npos, lnorm));
	//	float ramb = 5. * min(1., 1. + dot(pos, light)) * (normp * lightp < 0. ? .1 : .8) * abs(lightp) * abs(normp) * temp * temp;

		// Blend texturing, lighting and ring shadowing together.
		texColor += 0.
	//		- (ramb + 
			+ (
			gl_FrontLightProduct[0].ambient/* + globalAmbient*/
			+ gl_FrontLightProduct[0].diffuse
	//		* texture1D(tex1d, (length(q) - ringmin) / (ringmax - ringmin)));
			* max(sundot * (0. < dot(q, flight) ? texture1D(tex1d, (length(q) - ringmin) / (ringmax - ringmin)) : vec4(1)), 0.));
	}

	texColor *= 1. - max(0., .5 * float(cloudfunc(cloudtexture, vec3(gl_TexCoord[2]), view.z)));
	if(sundot < 0.1)
		texColor += textureCube(lightstexture, vec3(gl_TexCoord[0])) * min(.02, 5. * (-sundot + 0.1));
/*	texColor[0] = sqrt(texColor[0]);
	texColor[1] = sqrt(texColor[1]);
	texColor[2] = sqrt(texColor[2]);*/
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
//	texColor *= col;
	
//	vec4 envColor = textureCube(envmap, texCoord);
//	envColor[3] = col[3] / 2.;
//	gl_FragColor = (envColor + texColor);
	gl_FragColor = toneMapping(texColor);
}
