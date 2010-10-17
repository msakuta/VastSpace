uniform samplerCube texture;
uniform mat3 invEyeRot3x3;
uniform samplerCube bumptexture;
uniform samplerCube cloudtexture;
uniform float time;
 
varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

#include "shaders/earth_cloud_noise.fs"

#extension GL_EXT_gpu_shader4 : enable

vec3 innoise(vec3 v, int level){
	vec3 ret = vec3(0,0,0);
	float f[4];
	f[0] = mod(v[0], 65536.);
	f[1] = mod(v[1], 65536.);
	f[2] = mod(v[2], 65536.);
	f[3] = mod(time / pow(16., float(level)), 65536.);
	for(int y = 0; y < 2; y++) for(int x = 0; x < 2; x++) for(int z = 0; z < 2; z++) for(int t = 0; t < 2; t++){
		for(int i = 0; i < 3; i++){
			uint z0 =
				(4532635u * (uint(f[2]) + uint(z))) ^
				(9745270u * (uint(f[1]) + uint(y))) ^
				(5326436u * (uint(f[0]) + uint(x))) ^
				(3423251u * (uint(f[3]) + uint(t)));
			uint w = 0u;
			uint z1 = 36969u * (z0 & 65535u) + (z0 >> 16u);
			uint w1 = 18000u * (w & 65535u) + (w >> 16u);
			ret[i] +=
				(x == 0 ? 1. - mod(f[0], 1.) : mod(f[0], 1.)) *
				(y == 0 ? 1. - mod(f[1], 1.) : mod(f[1], 1.)) *
				(z == 0 ? 1. - mod(f[2], 1.) : mod(f[2], 1.)) *
				(t == 0 ? 1. - mod(f[3], 1.) : mod(f[3], 1.)) *
				float((z1 << 16u) + w1) / 4294967296.;
		}
	}
	return ret;
}

vec3 anoise1(vec3 v){
	return innoise(v, 0);
}

vec3 anoise2(vec3 v){
	return .5 * anoise1(v * 16.) + innoise(v, 1);
}

vec3 anoise3(vec3 v){
	return .5 * anoise2(v * 16.) + innoise(v, 2);
}

void main (void)
{
	vec3 normal = nrm;

	vec3 texnorm0 = vec3(textureCube(bumptexture, vec3(gl_TexCoord[0]))) - vec3(1., .5, .5);
	texnorm0[1] *= 5.;
	texnorm0[2] *= 5.;
	vec2 noise = .05 * vec2(anoise2(1000. * vec3(gl_TexCoord[0])));
	vec3 texnorm = (texa0 * (texnorm0[2] + noise[0]) + texa1 * (texnorm0[1] + noise[1]));
	vec3 fnormal = normalize(normalize(normal) + (texnorm) * 1.);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal) + .1);

	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));
//	texColor = vec4((anoise3(vec3(gl_TexCoord[0]) * 1000.) + vec3(1,1,1))/2, 0);
	texColor *= diffuse/* + ambient*/;
	float specular;
	float shininess;
	if(texColor[2] > texColor[0] + texColor[1])
		specular = 1., shininess = 20.;
	else
		specular = .3, shininess = 5.;
	texColor += specular * vec4(.75,.9,1,0) * pow(shininess * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);
	texColor *= 1. - textureCube(cloudtexture, vec3(gl_TexCoord[0]))[0] * (2. + cnoise3(400. * vec3(gl_TexCoord[0]))[0]) / 3.;
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
//	texColor *= col;
	
//	vec4 envColor = textureCube(envmap, texCoord);
//	envColor[3] = col[3] / 2.;
//	gl_FragColor = (envColor + texColor);
	gl_FragColor = texColor;
}
