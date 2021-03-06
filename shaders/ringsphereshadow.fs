#include "shaders/tonemap.fs"

/** \file ringsphereshadow.fs
 * \brief Sphere with ring's shadow projected.
 */

uniform sampler1D tex1d;
uniform samplerCube tex;
uniform float ambient;
uniform float ringmin, ringmax;
uniform vec3 ringnorm;

varying vec3 norm;

void main (void)
{
	vec4 light4 = gl_LightSource[0].position;

	// Lighting calculation
	vec3 light = normalize(vec3(light4));
	vec3 npos = normalize(norm);
	float lightangle = dot(light, npos);

	// Global ambient is not accumulated in gl_FrontLightProduct, so we must calculate it manually.
	vec4 globalAmbient = gl_LightModel.ambient * gl_FrontMaterial.ambient;

	// Aquire transformed vectors in eye space.
	vec3 pos = gl_NormalMatrix * vec3(gl_TexCoord[1]);
	vec3 lnorm = ringnorm;

	// Calculate projected point onto ring plane.
	float normp = dot(pos, lnorm);
	float lightp = dot(light, lnorm);
	vec3 q = pos - normp / lightp * light;

	// Calculate ring reflected ambient.
	float temp = 1. - abs(dot(npos, lnorm));
	float ramb = 5. * min(1., 1. + dot(pos, light)) * (normp * lightp < 0. ? .1 : .8) * abs(lightp) * abs(normp) * temp * temp;

	// Blend texturing, lighting and ring shadowing together.
	vec4 texColor = textureCube(tex, vec3(gl_TexCoord[0]))
		* (gl_FrontLightProduct[0].diffuse * ramb + 
		gl_FrontLightProduct[0].ambient + globalAmbient
		+ gl_FrontLightProduct[0].diffuse
		* max(lightangle * (0. < dot(q, light) ? texture1D(tex1d, (length(q) - ringmin) / (ringmax - ringmin)) : vec4(1)), 0.));

	gl_FragColor = toneMapping(texColor);
}
