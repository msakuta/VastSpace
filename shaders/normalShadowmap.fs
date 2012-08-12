uniform sampler2D texture;
uniform sampler2D texture2;
uniform bool textureEnable;
uniform bool texture2Enable;
//uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
uniform float ambient;
uniform int additive;
 
varying vec3 view;
varying vec3 nrm;
varying float diffuse[2];
//varying vec4 col;

float shadowMapIntensity(float offset);

void main (void)
{
    float offsetEnv;

	vec3 normal = nrm;

	vec3 fnormal = normalize(normal)/* + invEyeRot3x3 * vec3(texture2D(nrmmap, vec2(gl_TexCoord[0]))) * .2*/;
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	
	vec3 fview = normalize(view);

	float fdiffuse = max(0.001, dot(flight, fnormal));

	float shadow = shadowMapIntensity(1. / fdiffuse);

	vec3 lightProduct = (shadow * .8 + .2) * gl_FrontLightProduct[0].diffuse.xyz * diffuse[0]
			+ gl_FrontLightProduct[0].ambient.xyz
			+ gl_FrontLightProduct[1].diffuse.xyz * diffuse[1] + gl_FrontLightProduct[1].ambient.xyz
			+ gl_FrontLightModelProduct.sceneColor.xyz;

//	vec4 texColor = shadow;
	vec4 texColor = !textureEnable ? vec4(1,1,1,1) : texture2D(texture, vec2(gl_TexCoord[0]));

	// Apply the second texture
	if(.5 < gl_TextureEnvColor[0].y)
		texColor *= texture2D(texture2, vec2(gl_TexCoord[1]));

	texColor.xyz *= gl_FrontMaterial.emission.xyz + (vec3(1,1,1) - gl_FrontMaterial.emission.xyz)
		* min(vec3(1,1,1), lightProduct);

	gl_FragColor = texColor;
}
