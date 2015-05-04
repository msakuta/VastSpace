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
uniform int lightCount;
uniform mat3 rotation;
// uniform float height; // Defined in earth_cloud_noise.fs

varying vec3 view;
varying vec3 nrm;
varying vec4 col;

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

	// Diminishing details when the viewpoint is far from surface.
	// It should use pixel-wise distance from viewpoint, but gl_FragCoord.z seems not to scale,
	// so we settle for given uniform variable.
	// The uniform variable height is measured in meters, so we convert the value to reasonable dynamic range.
	float rheight = height / 1000.;

	// Accumulate multiple noises to reduce artifacts caused by finite texture size.
	return 0.2 * (
		(1.0 * f2 * (texture3D(noise3D, 128. * v) - vec4(.5)) + // Fine noise
		1.0 * f2 * (texture3D(noise3D, 128. * v2) - vec4(.5))) / (1. + 2. * rheight) + // Fine rotated noise
		(texture3D(noise3D, 32. * v) - vec4(0.5)) / (1. + 1. * rheight) + // High octave noise
		(texture3D(noise3D, 8. * v) - vec4(0.5)) / (1. + 0.2 * rheight) // Very high octave noise
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
		specular = 3.5, shininess = 50., waving = true;
	else
		specular = 0.25, shininess = 2., waving = false;
	float sundot = dot(flight, normal);
	float dawness = sundot * 8.;
	dawness = 1. - exp(-dawness * dawness);
	vec4 vshininess = vec4(.75, .2 + .6 * dawness, .3 + .7 * dawness, 0.);


	// Obtain ocean noise to add to normal vector.
	// The noise position moves over time, so it would be a random source.
	// But do not add in case of rendering land.
	vec3 noiseInput = normalize(texCoord);
	if(waving)
		noiseInput += 2e-6 * noisePos;
	vec4 noise = ocean(noiseInput);


	// Disable bump map texture since new inflated cube algorithm incorpolates terrain
	// geometry which affects normal vector.  Probably we could use bump maps in lower LODs,
	// but it would be another story.
//	vec3 texsample = textureCube(bumptexture, texCoord);
//	vec3 texnorm0 = texsample - vec3(0.5, 0.5, 0.5) + noise.xyz;
//	vec3 fnormal = normalize(normal + rotation * texnorm0);
	vec3 fnormal = normalize(normal + noise.xyz);

	vec3 fview = normalize(view);

	// Obtain cloud thickness above. TODO: project sun's ray of light onto sphere.
	float cloud = cloudfunc(cloudtexture, vec3(gl_TexCoord[2]), view.z).a;
	// How diffuse light is blocked by cloud. The coefficient of cloud looks better to be less than 1.
	float cloudBlock = 1. - 0.75 * cloud;

	// If you're far from the ocean surface, small waves are averaged and apparent shininess decreases.
	float innerShininess = shininess * 50.;
	innerShininess /= min(50., 1. - 0.2 * view.z);

	vec3 reflectDirection = reflect(fview, fnormal);

	vec3 diffuse = vec3(0,0,0);
	vec3 ambient = vec3(0,0,0);
	vec3 specAccum = vec3(0,0,0);
	for(int i = 0; i < lightCount; i++){
		vec3 flight = normalize(gl_LightSource[i].position.xyz);

		vec3 ldiffuse = max(vec3(0,0,0), dot(flight, fnormal) * gl_LightSource[i].diffuse.xyz);

		// Dot product of light and geometry normal.
		float dtn = dot(flight, normal);
		// The ambient factor is calculated with the same formula as the atmosphere's.
		float ambf = 0. < dtn ? 1. : pow(1. + dtn, 8.)/* max(0., min(1., (dtn + 0.1) / 0.2))*/;

		// Diffuse strength should be scaled with ambf too, or subtle artifacts appear in sunrise and sunset.
		ldiffuse *= ambf * cloudBlock;

		float reflection = 1. - dot(flight, reflectDirection);

		// Outer specular highlight
		vec3 lspecular = vec3(pow(shininess * reflection + 1., -2.));
		// Another specular for the light source's direct ray
		lspecular += vec3(pow(innerShininess * reflection + 1., -2.));

		// Specular reflection won't reach the night side of the Earth.
		specAccum += gl_LightSource[i].diffuse.xyz * ambf * cloudBlock * specular * max(.1, min(1., dtn))
			* lspecular;

		diffuse += ldiffuse;
		ambient += gl_LightSource[i].ambient.xyz + gl_LightSource[i].diffuse.xyz * (0.15 * ambf * (1. - 0.5 * cloud));
	}

	texColor.xyz *= (diffuse + ambient) * (1. + 2. * noise.w);
	texColor.xyz += specAccum;

	if(sundot < 0.1)
		texColor += textureCube(lightstexture, vec3(gl_TexCoord[0])) * min(.02, 5. * (-sundot + 0.1));
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
	
	gl_FragColor = toneMapping(texColor);
}
