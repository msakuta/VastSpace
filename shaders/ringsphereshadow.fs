// ringshadow.fs

uniform sampler1D tex1d;
uniform samplerCube tex;
uniform float ambient;
uniform float ringmin, ringmax;
uniform vec3 ringnorm;

varying vec3 norm;
//varying vec3 pos;

void main (void)
{
	vec4 light4 = gl_LightSource[0].position;
	light4[3] = 0.;
	vec3 light = vec3(light4);
	light = normalize(light);
	vec3 npos = normalize(gl_NormalMatrix * norm);
	float lightangle = dot(light, npos);
	vec4 globalAmbient = gl_LightModel.ambient * gl_FrontMaterial.ambient;
	vec3 localight = normalize(/*transpose(gl_NormalMatrix) * */light);
	vec3 pos = gl_NormalMatrix * vec3(gl_TexCoord[1]);
	vec3 lnorm = gl_NormalMatrix * ringnorm;
	vec3 q = pos - dot(pos, lnorm) / dot(localight, lnorm) * localight;
	gl_FragColor = textureCube(tex, vec3(gl_TexCoord[0]))
		* (gl_FrontLightProduct[0].ambient + globalAmbient
		+ gl_FrontLightProduct[0].diffuse
		* max(lightangle * (0. < dot(q, localight) ? texture1D(tex1d, (length(q) - ringmin) / (ringmax - ringmin)) : vec4(1)), 0.));
//	gl_FragColor[3] = texture1D(tex1d, gl_TexCoord[0][0])[3] * .5;
//	gl_FragColor *= texture1D(tex1d, (length(pos) - ringmin) / (ringmax - ringmin));
//	gl_FragColor[3] *= .5;
}
