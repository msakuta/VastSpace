uniform sampler2D texture;
uniform sampler2D texture2;
uniform bool textureEnable;
uniform bool texture2Enable;
//uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
uniform float ambient;
uniform int additive;

varying vec4 view;
varying vec3 nrm;
varying float diffuse[2];
//varying vec4 col;

#include "shaders/shadowmap.fs"
#include <shaders/tonemap.fs>

void main (void)
{
	float offsetEnv;

	vec3 normal = nrm;

	vec3 fnormal = normalize(normal)/* + invEyeRot3x3 * vec3(texture2D(nrmmap, vec2(gl_TexCoord[0]))) * .2*/;
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 fview = normalize(view.xyz);

	float fdiffuse = max(0.001, dot(flight, fnormal));

	float shadow = shadowMapIntensity(1. / fdiffuse);

	vec3 lightProduct = (shadow * .8 + .2) * gl_FrontLightProduct[0].diffuse.xyz * diffuse[0]
			+ gl_FrontLightProduct[0].ambient.xyz
			+ gl_FrontLightProduct[1].diffuse.xyz * diffuse[1] + gl_FrontLightProduct[1].ambient.xyz
			+ gl_FrontLightModelProduct.sceneColor.xyz;

	if(0. < shadow && 0. < length(gl_FrontLightProduct[0].specular)){
		vec3 lightVec = normalize((gl_LightSource[0].position * view.w - gl_LightSource[0].position.w * view).xyz);
		float attenuation = shadow;
		vec3 viewVec = normalize(-view.xyz);
		vec3 halfway = normalize(lightVec + viewVec);
		float specular = pow(max(dot(fnormal, halfway), 0.0), gl_FrontMaterial.shininess);
		lightProduct += gl_FrontLightProduct[0].specular.xyz * specular * attenuation;
	}

//	vec4 texColor = shadow;
	vec4 texColor = !textureEnable ? vec4(1,1,1,1) : texture2D(texture, vec2(gl_TexCoord[0]));

	// Apply the second texture
	if(texture2Enable)
		texColor *= texture2D(texture2, vec2(gl_TexCoord[1]));

	texColor.xyz *= gl_FrontMaterial.emission.xyz + (vec3(1,1,1) - gl_FrontMaterial.emission.xyz)
		* lightProduct;

	gl_FragColor = toneMapping(texColor);
}
