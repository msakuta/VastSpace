// ringsphereshadow.fs

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
	vec3 npos = normalize(gl_NormalMatrix * norm);
	float lightangle = dot(light, npos);

	// Global ambient is not accumulated in gl_FrontLightProduct, so we must calculate it manually.
	vec4 globalAmbient = gl_LightModel.ambient * gl_FrontMaterial.ambient;

	// Aquire transformed vectors in eye space.
	vec3 pos = gl_NormalMatrix * vec3(gl_TexCoord[1]);
	vec3 lnorm = gl_NormalMatrix * ringnorm;

	// Calculate projected point onto ring plane.
	vec3 q = pos - dot(pos, lnorm) / dot(light, lnorm) * light;

	// Blend texturing, lighting and ring shadowing together.
	gl_FragColor = textureCube(tex, vec3(gl_TexCoord[0]))
		* (gl_FrontLightProduct[0].ambient + globalAmbient
		+ gl_FrontLightProduct[0].diffuse
		* max(lightangle * (0. < dot(q, light) ? texture1D(tex1d, (length(q) - ringmin) / (ringmax - ringmin)) : vec4(1)), 0.));
}
