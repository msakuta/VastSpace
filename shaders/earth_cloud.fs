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
varying vec3 tlight;

//#include "shaders/earth_cloud_noise.fs"
vec4 cloudfunc(samplerCube texture, vec3 v, float z);

/*
vec3 cinnoise(vec3 v, int level){
	vec3 ret = vec3(0,0,0);
	float f[3];
	f[0] = mod(v[0], 65536.);
	f[1] = mod(v[1], 65536.);
	f[2] = mod(v[2], 65536.);
	for(uint y = 0; y < 2; y++) for(uint x = 0; x < 2; x++) for(uint z = 0; z < 2; z++){
		for(int i = 0; i < 3; i++){
			uint z0 =
				(4532635 * (uint(f[2]) + z)) ^
				(9745270 * (uint(f[1]) + y)) ^
				(5326436 * (uint(f[0]) + x));
			uint w = 0;
			uint z1 = 36969*(z0&65535)+(z0>>16);
			uint w1 = 18000*(w&65535)+(w>>16);
			ret[i] +=
				(!x ? 1. - mod(f[0], 1.) : mod(f[0], 1.)) *
				(!y ? 1. - mod(f[1], 1.) : mod(f[1], 1.)) *
				(!z ? 1. - mod(f[2], 1.) : mod(f[2], 1.)) *
				float((z1<<16)+(w1)) / 4294967296.;
		}
	}
	return ret;
}

vec3 cnoise1(vec3 v){
	return cinnoise(v, 0);
}

vec3 cnoise2(vec3 v){
	return .5 * cnoise1(v * 2.) + cinnoise(v, 1);
}

vec3 cnoise3(vec3 v){
	return .5 * cnoise2(v * 2.) + cinnoise(v, 2);
}*/

void main (void)
{
	vec3 normal = nrm;
	vec3 fnormal = normalize(normal);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	float diffuse = max(0., dot(flight, fnormal) + .2);

	float dawness = dot(flight, fnormal) * 8.;
	dawness = 1. - exp(-dawness * dawness);
	float f = cloudfunc(texture, vec3(gl_TexCoord[0]) - normalize(tlight) * .0005, view.z)[3];
	vec4 texColor = vec4(.6, .5, .4, f);
//	vec4 texColor = (textureCube(texture, vec3(gl_TexCoord[0])) - cnoise4(400. * vec3(gl_TexCoord[0]))[0] / 1.75) / 1.;
	f = cloudfunc(texture, vec3(gl_TexCoord[0]), view.z)[3];
	texColor = texColor * (1. - f) + f * vec4(1, dawness, dawness, 1);
//	vec4 texColor = texture3D(noise3D, vec3(gl_TexCoord[0]));
//	texColor = texColor * texColor[3] + (1. - texColor[3]) * vec4(1, 0, 0, cloudfunc(texture, vec3(gl_TexCoord[0]) - flight * 1e-3, view.z)[3]);
	texColor *= diffuse /** anoise2(1000. * vec3(gl_TexCoord[0]))[0]*/;
//	texColor[0] = texColor[1] = texColor[2] = 1.;
	gl_FragColor = texColor;
}
