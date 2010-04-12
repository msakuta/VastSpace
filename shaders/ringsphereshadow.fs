// ringshadow.fs

uniform sampler1D tex1d;
uniform samplerCube tex;
uniform float ambient;
uniform float ringmin, ringmax;

varying vec3 pos;

void main (void)
{
	vec4 light4 = gl_LightSource[0].position;
	light4[3] = 0;
	vec3 light = vec3(transpose(gl_ModelViewMatrix) * light4);
	light = normalize(light);
	vec3 npos = normalize(pos);
	float lightangle = dot(light, npos);
	gl_FragColor = textureCube(tex, gl_TexCoord[0]) *
		texture1D(tex1d, gl_TexCoord[1][0]) * (ambient + (1. - ambient) * (lightangle < 0 ? 0 : lightangle));
//	gl_FragColor[3] = texture1D(tex1d, gl_TexCoord[0][0])[3] * .5;
//	gl_FragColor *= texture1D(tex1d, (length(pos) - ringmin) / (ringmax - ringmin));
//	gl_FragColor[3] *= .5;
}
