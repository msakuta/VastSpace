uniform sampler2D texture;
uniform sampler2D texture2;
//uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
uniform sampler2DShadow shadowmap;
uniform sampler2DShadow shadowmap2;
uniform sampler2DShadow shadowmap3;
uniform float ambient;
 
varying vec3 view;
//varying vec3 nrm;
varying float diffuse;
//varying vec4 col;

void main (void)
{
    float offsetEnv;

//	vec3 normal = nrm;

//	vec3 fnormal = normalize(normal)/* + invEyeRot3x3 * vec3(texture2D(nrmmap, vec2(gl_TexCoord[0]))) * .2*/;
//	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	
	vec3 fview = normalize(view);

//	float diffuse = max(0., dot(flight, fnormal));

	vec4 shadow;
	if(	   0.001 < gl_TexCoord[4].x && gl_TexCoord[4].x < 0.999
		&& 0.001 < gl_TexCoord[4].y && gl_TexCoord[4].y < 0.999
		&& 0.001 < gl_TexCoord[4].z && gl_TexCoord[4].z < 0.999)
		shadow = shadow2DProj(shadowmap3, gl_TexCoord[4]);
	else if(0.001 < gl_TexCoord[3].x && gl_TexCoord[3].x < 0.999
		&& 0.001 < gl_TexCoord[3].y && gl_TexCoord[3].y < 0.999
		&& 0.001 < gl_TexCoord[3].z && gl_TexCoord[3].z < 0.999)
		shadow = shadow2DProj(shadowmap2, gl_TexCoord[3]);
	else{
		float shadowedge = min(gl_TexCoord[2].x, 1. - gl_TexCoord[2].x);
		shadowedge = min(shadowedge, min(gl_TexCoord[2].y, 1. - gl_TexCoord[2].y));
		shadowedge = min(shadowedge, min(gl_TexCoord[2].z, 1. - gl_TexCoord[2].z));
		shadowedge = 1. - min(1., 10. * shadowedge);
		shadow = shadow2DProj(shadowmap, gl_TexCoord[2]) * (1. - shadowedge) + shadowedge;
	}


//	vec4 texColor = shadow;
	vec4 texColor = gl_TextureEnvColor[0].x < .5 ? vec4(1,1,1,1) : texture2D(texture, vec2(gl_TexCoord[0]));

	// Apply the second texture
	if(.5 < gl_TextureEnvColor[0].y)
		texColor *= texture2D(texture2, vec2(gl_TexCoord[1]));

	vec3 lightProduct = (shadow[3] * .8 + .2) * gl_FrontLightProduct[0].diffuse.xyz * diffuse
			+ gl_FrontLightProduct[0].ambient.xyz
			+ gl_FrontLightModelProduct.sceneColor.xyz;

	texColor.xyz *= gl_FrontMaterial.emission.xyz + (vec3(1,1,1) - gl_FrontMaterial.emission.xyz)
		* min(vec3(1,1,1), lightProduct);

//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
//	texColor *= col;
	
//	vec4 envColor = textureCube(envmap, texCoord);
//	envColor[3] = col[3] / 2.;
//	gl_FragColor = (envColor + texColor);
	gl_FragColor = texColor;
}
