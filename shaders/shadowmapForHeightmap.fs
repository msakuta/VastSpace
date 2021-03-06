uniform sampler2D texture;
uniform sampler2D texture2;
uniform sampler2D texture3;
//uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
uniform sampler2DShadow shadowmap;
uniform sampler2DShadow shadowmap2;
uniform sampler2DShadow shadowmap3;
uniform float ambient;
 
varying vec3 view;
varying vec3 nrm;
//varying float diffuse;
//varying vec4 col;

void main (void)
{
    float offsetEnv;

	vec3 normal = nrm;

	vec3 fnormal = /*normalize(normal)*/ + normalize(invEyeRot3x3 * vec3(texture2D(texture3, vec2(gl_TexCoord[5]))));
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	
	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal));

	vec4 shadow =
		   0.001 < gl_TexCoord[4].x && gl_TexCoord[4].x < 0.999
		&& 0.001 < gl_TexCoord[4].y && gl_TexCoord[4].y < 0.999
		&& 0.001 < gl_TexCoord[4].z && gl_TexCoord[4].z < 0.999
		? shadow2DProj(shadowmap3, gl_TexCoord[4]) :
		   0.001 < gl_TexCoord[3].x && gl_TexCoord[3].x < 0.999
		&& 0.001 < gl_TexCoord[3].y && gl_TexCoord[3].y < 0.999
		&& 0.001 < gl_TexCoord[3].z && gl_TexCoord[3].z < 0.999
		? shadow2DProj(shadowmap2, gl_TexCoord[3]) : shadow2DProj(shadowmap, gl_TexCoord[2]);

//	vec4 texColor = shadow;
	vec4 texColor = gl_TextureEnvColor[0].x < .5 ? vec4(1,1,1,1) : texture2D(texture, vec2(gl_TexCoord[0]));

	// Apply the second texture
	if(.5 < gl_TextureEnvColor[0].y)
		texColor *= texture2D(texture2, vec2(gl_TexCoord[1]));

	texColor.xyz *= gl_FrontMaterial.emission + (1. - gl_FrontMaterial.emission)
		* min(1., (shadow[3] * .8 + .2) * gl_FrontLightProduct[0].diffuse * diffuse
			+ gl_FrontLightProduct[0].ambient + gl_FrontLightModelProduct.sceneColor);

//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
//	texColor *= col;
	
//	vec4 envColor = textureCube(envmap, texCoord);
//	envColor[3] = col[3] / 2.;
//	gl_FragColor = (envColor + texColor);
	gl_FragColor = texColor;
}
