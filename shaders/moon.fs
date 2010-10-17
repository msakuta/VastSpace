uniform samplerCube texture;
//uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform samplerCube bumptexture;
//uniform float ambient;
 
varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

void main (void)
{
//    float offsetEnv;

	vec3 normal = normalize(nrm);

	vec3 texnorm0 = vec3(textureCube(bumptexture, vec3(gl_TexCoord[0]))) - vec3(1., .5, .5);
	texnorm0[1] *= 5.;
	texnorm0[2] *= 5.;
	vec3 texnorm = ((texa0) * texnorm0[2] + (texa1) * texnorm0[1]);
//	vec3 texnorm = normalize(nrm + texa1 + texa0);
//	vec3 texnorm = vec3(texnorm0[2], texnorm0[1], texnorm0[0]);
	vec3 fnormal = normalize(normalize(normal) + (texnorm) * 1.);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal));
//	diffuse = 1.;

	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));
//	texColor = vec4(fnormal, 0);
	texColor *= diffuse/* + ambient*/;
//	texColor += vec4(.75,.9,1,0) * pow(30. * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
//	texColor *= col;
	
//	vec4 envColor = textureCube(envmap, texCoord);
//	envColor[3] = col[3] / 2.;
//	gl_FragColor = (envColor + texColor);
	gl_FragColor = texColor;
}
