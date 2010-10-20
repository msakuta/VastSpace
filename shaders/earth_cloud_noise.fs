#extension GL_EXT_gpu_shader4 : enable

vec3 cinnoise(vec3 v, int level){
	vec3 ret = vec3(0,0,0);
	float f[3];
	f[0] = mod(v[0], 65536.);
	f[1] = mod(v[1], 65536.);
	f[2] = mod(v[2], 65536.);
	for(int y = 0; y < 2; y++) for(int x = 0; x < 2; x++) for(int z = 0; z < 2; z++){
		for(int i = 0; i < 1; i++){
			uint z0 =
				(4532635u * (uint(f[2]) + uint(z))) ^
				(9745270u * (uint(f[1]) + uint(y))) ^
				(5326436u * (uint(f[0]) + uint(x)));
			uint w = 0u;
			uint z1 = 36969u * (z0 & 65535u) + (z0 >> 16u);
			uint w1 = 18000u * (w & 65535u) + (w >> 16u);
			ret[i] +=
				(x == 0 ? 1. - mod(f[0], 1.) : mod(f[0], 1.)) *
				(y == 0 ? 1. - mod(f[1], 1.) : mod(f[1], 1.)) *
				(z == 0 ? 1. - mod(f[2], 1.) : mod(f[2], 1.)) *
				float((z1 << 16u) + w1) / 4294967296.;
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
}

vec3 cnoise4(vec3 v){
	return .5 * cnoise3(v * 2.) + cinnoise(v, 3);
}

vec3 cnoise5(vec3 v){
	return .5 * cnoise4(v * 2.) + cinnoise(v, 3);
}

uniform sampler3D noise3D;
uniform float height;

vec4 cloudfunc(samplerCube texture, vec3 v, float z){
//	return (textureCube(texture, vec3(v)) - cnoise5(400. * normalize(v))[0] / 1.75 / 2.) / 1.;
	float f2 = min(1., 20. / height);
	return textureCube(texture, v) * (2.0 - 3.0 * f2 * (texture3D(noise3D, 128. * normalize(v))[3] - .5) - 1. * texture3D(noise3D, 32. * normalize(v))[3] - texture3D(noise3D, 8. * normalize(v))[3]);
//	vec4 texColor = (textureCube(texture, vec3(gl_TexCoord[0])) - cnoise4(400. * vec3(gl_TexCoord[0]))[0] / 1.75) / 1.;
}

