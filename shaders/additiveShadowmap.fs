uniform sampler2D texture;
uniform sampler2D texture2;
//uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
uniform float ambient;
uniform int additive;
 
varying vec3 view;
//varying vec3 nrm;
varying float diffuse;
//varying vec4 col;

float shadowMapIntensity();

void main (void)
{
    float offsetEnv;

//	vec3 normal = nrm;

//	vec3 fnormal = normalize(normal)/* + invEyeRot3x3 * vec3(texture2D(nrmmap, vec2(gl_TexCoord[0]))) * .2*/;
//	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	
	vec3 fview = normalize(view);

//	float diffuse = max(0., dot(flight, fnormal));

	float shadow = shadowMapIntensity();

	vec3 lightProduct = (shadow * .8 + .2) * gl_FrontLightProduct[0].diffuse.xyz * diffuse
			+ gl_FrontLightProduct[0].ambient.xyz
			+ gl_FrontLightModelProduct.sceneColor.xyz;

//	vec4 texColor = shadow;
	vec4 texColor = gl_TextureEnvColor[0].x < .5 ? vec4(1,1,1,1) : texture2D(texture, vec2(gl_TexCoord[0]));

	texColor.xyz = (gl_FrontMaterial.emission.xyz + texColor.xyz) + (vec3(1,1,1) - (gl_FrontMaterial.emission.xyz) + texColor.xyz)
		* min(vec3(1,1,1), lightProduct);

	// Apply the second texture
	if(.5 < gl_TextureEnvColor[0].y)
		texColor *= texture2D(texture2, vec2(gl_TexCoord[1]));

	gl_FragColor = texColor;
}
