uniform samplerCube envmap;
uniform mat3 invEyeRot3x3;
uniform sampler2D nrmmap;
 
varying vec3 view;
varying vec3 nrm;
varying vec4 col;

void main (void)
{
    float offsetEnv;
    vec4 matColor;

	vec3 normal = nrm;

	vec3 fnormal = invEyeRot3x3 * normalize(normal);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	
	vec3 fview = normalize(view);

	float diffuse = dot(flight, fnormal);

	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal);

	vec4 envColor = textureCube(envmap, texCoord);

	gl_FragColor = envColor + col;
}
