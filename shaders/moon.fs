uniform samplerCube texture;
uniform mat3 invEyeRot3x3;
uniform samplerCube bumptexture;
 
varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

void main (void)
{
	vec3 normal = normalize(nrm);
	vec3 texnorm0 = vec3(textureCube(bumptexture, vec3(gl_TexCoord[0]))) - vec3(1., .5, .5);
	texnorm0[1] *= 5.;
	texnorm0[2] *= 5.;
	vec3 texnorm = ((texa0) * texnorm0[2] + (texa1) * texnorm0[1]);
	vec3 fnormal = normalize(normalize(normal) + (texnorm) * 1.);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal));

	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));
	texColor *= diffuse/* + ambient*/;
	texColor[3] = 1.;
	gl_FragColor = texColor;
}
