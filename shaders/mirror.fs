uniform sampler2D texture;
uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
 
varying vec3 view;
varying vec3 nrm;
varying vec4 col;

void main (void)
{
    float offsetEnv;

	vec3 normal = nrm;

	vec3 fnormal = invEyeRot3x3 * normalize(normal);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	
	vec3 fview = normalize(view);

	float diffuse = dot(flight, fnormal);

	vec4 texColor = texture2D(texture, vec2(gl_TexCoord[0]));
	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .025 * vec3(texColor);
	texColor *= col;
	texColor[3] = col[3] / 2.;

	vec4 envColor = textureCube(envmap, texCoord);
	envColor[3] = col[3] / 2.;
	gl_FragColor = (envColor + texColor);
}
