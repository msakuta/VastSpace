/** \file tonemap.fs
 * \brief The exposure adjustment and tone mapping shader function.
 */
uniform float exposure;
uniform int tonemap;

const float brightMax = 5.;

vec4 toneMapping(vec4 texColor)
{
	if(0 == tonemap)
		texColor.xyz *= exposure;
	else{
		vec3 fv = texColor.rgb * exposure;
		texColor.xyz = fv * (fv/brightMax + 1.0) / (fv + 1.0);
//		texColor.xyz = fv / (fv + 1.0);
	}
	return texColor;
}

vec4 toneMappingAlpha(vec4 texColor)
{
	if(0 == tonemap)
		texColor.rgb *= exposure;
	else{
		vec3 fv = texColor.rgb * exposure;
		texColor.rgb = fv * (fv/brightMax + 1.0) / (fv + 1.0);
	}
	float saturated = 0.;
	for(int i = 0; i < 3; i++)
		saturated += max(0., texColor[i] - 1.);
	// Too bright color saturates the image sensor so that it hides things behind.
	if(1. < saturated){
		texColor.a *= saturated;
	}
	return texColor;
}
