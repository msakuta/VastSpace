uniform samplerCube texture;
uniform mat3 invEyeRot3x3;
uniform samplerCube bumptexture;
uniform samplerCube cloudtexture;
uniform samplerCube lightstexture;
uniform float time;
 
varying vec3 view;
varying vec3 nrm;
varying vec4 col;
varying vec3 texa0; // texture axis component 0
varying vec3 texa1; // texture axis component 1

// If GLSL standard states no #include is allowed, how could functions be shared?
// We cannot share even prototypes.
//#include "shaders/earth_cloud_noise.fs"
vec4 cloudfunc(samplerCube texture, vec3 v, float z);

#extension GL_EXT_gpu_shader4 : enable

bool waving;

vec3 innoise(vec3 v, int level){
	vec3 ret = vec3(0,0,0);
	float f[4];
	f[0] = mod(v[0], 65536.);
	f[1] = mod(v[1], 65536.);
	f[2] = mod(v[2], 65536.);
	f[3] = mod(time / pow(2., float(level)), 65536.);
	for(int x = 0; x < 2; x++){
		float fx = x == 0 ? 1. - mod(f[0], 1.) : mod(f[0], 1.);
		for(int y = 0; y < 2; y++){
			float fy = y == 0 ? 1. - mod(f[1], 1.) : mod(f[1], 1.);
			for(int z = 0; z < 2; z++){
				float fz = z == 0 ? 1. - mod(f[2], 1.) : mod(f[2], 1.);
				for(int t = 0; t < 2; t++){
					uint z0 =
						(532516436u * (uint(f[0]) + uint(x))) ^
						(974594270u * (uint(f[1]) + uint(y))) ^
						(442532635u * (uint(f[2]) + uint(z))) ^
						(waving ? 342943251u * (uint(f[3]) + uint(t)) : 0u) /*^
						(545389780u * uint(i))*/;
					for(int i = 0; i < 3; i++){
			//			uint w = 0u;
						z0 = 36969u * (z0 & 65535u) + (z0 >> 16u);
			//			uint w1 = 18000u * (w & 65535u) + (w >> 16u);

						// I cannot believe the compiler cannot optimize the inside-loop variables.
						ret[i] +=
							fx *
		//					(x == 0 ? 1. - mod(f[0], 1.) : mod(f[0], 1.)) *
							fy *
		//					(y == 0 ? 1. - mod(f[1], 1.) : mod(f[1], 1.)) *
							fz *
		//					(z == 0 ? 1. - mod(f[2], 1.) : mod(f[2], 1.)) *
							(t == 0 ? 1. - mod(f[3], 1.) : mod(f[3], 1.)) *
							float(z0 & 65535u) / 65536.;
					}
				}
			}
		}
	}
	return ret;
}

vec3 anoise1(vec3 v){
	return innoise(v, 0);
}

vec3 anoise2(vec3 v){
	return 2. * anoise1(v * 16.) + innoise(v, 1);
}

vec3 anoise3(vec3 v){
	return 1. * anoise2(v * 16.) + innoise(v, 2);
}

void main (void)
{
	vec3 normal = normalize(nrm);
	vec3 flight = normalize(gl_LightSource[0].position.xyz);
	vec3 texCoord = vec3(gl_TexCoord[0]);

	vec4 texColor = textureCube(texture, texCoord);
	float specular;
	float shininess;
	if(texColor[2] > texColor[0] + texColor[1])
		specular = 1., shininess = 20., waving = true;
	else
		specular = .3, shininess = 5., waving = false;
	float sundot = dot(flight, normal);
	float dawness = sundot * 8.;
	specular *= max(.1, min(1., dawness));
	dawness = 1. - exp(-dawness * dawness);
	vec4 vshininess = vec4(.75, .2 + .6 * dawness, .3 + .7 * dawness, 0.);

	vec3 texnorm0 = vec3(textureCube(bumptexture, texCoord)) - vec3(1., .5, .5);
	texnorm0[1] *= 5.;
	texnorm0[2] *= 5.;
	vec3 wavedir = vec3(1,1,0);
	vec3 wavedir2 = vec3(.25,.5,1);
	vec3 noise = .05 * vec3(anoise2(8. * 1000. * texCoord))
		+ sin(16. * 8. * 1000. * dot(wavedir, texCoord) + time) * wavedir * .05
		+ sin(16. * 8. * 1000. * dot(wavedir2, texCoord) + time) * wavedir2 * .05;
	vec3 texnorm = (texa0 * (texnorm0[2]) + texa1 * (texnorm0[1]) + noise);
	vec3 fnormal = normalize((normal) + (texnorm) * 1.);

	vec3 fview = normalize(view);

	float diffuse = max(0., dot(flight, fnormal) + .1);

//	texColor = vec4((anoise3(vec3(gl_TexCoord[0]) * 1000.) + vec3(1,1,1))/2, 0);
	texColor *= diffuse/* + ambient*/;
	texColor += specular * vshininess * pow(shininess * (1. - dot(flight, (reflect(invEyeRot3x3 * fview, fnormal)))) + 1., -2.);
	texColor *= 1. - max(0., .5 * float(cloudfunc(cloudtexture, vec3(gl_TexCoord[2]), view.z)));
	if(sundot < 0.1)
		texColor += textureCube(lightstexture, vec3(gl_TexCoord[0])) * min(.5, 5. * (-sundot + 0.1));
/*	texColor[0] = sqrt(texColor[0]);
	texColor[1] = sqrt(texColor[1]);
	texColor[2] = sqrt(texColor[2]);*/
	texColor[3] = 1.;
//	vec3 texCoord = reflect(invEyeRot3x3 * fview, fnormal) + .5 * vec3(texColor);
//	texColor *= col;
	
//	vec4 envColor = textureCube(envmap, texCoord);
//	envColor[3] = col[3] / 2.;
//	gl_FragColor = (envColor + texColor);
	gl_FragColor = texColor;
}
