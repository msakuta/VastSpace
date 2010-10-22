
uniform sampler3D noise3D;
uniform float height;

vec4 cloudfunc(samplerCube texture, vec3 v, float z){
//	return (textureCube(texture, vec3(v)) - cnoise5(400. * normalize(v))[0] / 1.75 / 2.) / 1.;
	float f2 = min(1., 20. / height);
	return textureCube(texture, v) * (2.0 - 3.0 * f2 * (texture3D(noise3D, 128. * normalize(v))[3] - .5) - 1. * texture3D(noise3D, 32. * normalize(v))[3] - texture3D(noise3D, 8. * normalize(v))[3]);
//	vec4 texColor = (textureCube(texture, vec3(gl_TexCoord[0])) - cnoise4(400. * vec3(gl_TexCoord[0]))[0] / 1.75) / 1.;
}

