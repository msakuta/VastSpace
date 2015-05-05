uniform samplerCube texture;
uniform samplerCube bumptexture;
uniform int lightCount;
uniform mat3 rotation;
#  include "shaders/tonemap.fs"

void main (void)
{
	vec3 texsample = textureCube(bumptexture, gl_TexCoord[0].xyz).xyz;
	vec3 texnorm0 = texsample - vec3(0.5, .5, .5);
	vec3 fnormal = normalize(rotation * texnorm0);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);

	vec3 diffuse = vec3(0,0,0);
	for(int i = 0; i < lightCount; i++){
		vec3 flight = normalize(gl_LightSource[i].position.xyz);

		diffuse += max(vec3(0,0,0), dot(flight, fnormal) * gl_LightSource[i].diffuse.xyz);
	}

	vec4 texColor = textureCube(texture, vec3(gl_TexCoord[0]));
	texColor.xyz *= diffuse/* + ambient*/;
	texColor[3] = 1.;
	gl_FragColor = toneMapping(texColor);

	// Debug outputs
//	gl_FragColor.xyz = gl_TexCoord[0].xyz;
//	gl_FragColor.xyz = fnormal;
//	gl_FragColor.xyz = texsample1;
//	gl_FragColor.xyz = vec3(textureCube(bumptexture, gl_TexCoord[0].xyz));
}
