/** \file tonemap.fs
 * \brief The exposure adjustment and tone mapping shader function.
 */
uniform float exposure;
uniform int tonemap;

vec4 toneMapping(vec4 texColor)
{
	if(0 == tonemap)
		texColor.xyz *= exposure;
	else{
		float brightMax = 5.;
		vec3 fv = texColor.rgb * exposure;
		texColor.xyz = fv * (fv/brightMax + 1.0) / (fv + 1.0);
//		texColor.xyz = fv / (fv + 1.0);
	}
	return texColor;
}

vec4 toneMappingAlpha(vec4 texColor)
{
/*	float a = texColor.a;

	if(0 == tonemap)
		a *= exposure;
	else{
		float brightMax = 5.;
		a *= exposure;
		a = a * (a / brightMax + 1.0) / (a + 1.0);
	}

	texColor.a = a;
	if(a < 1)
		return texColor;*/

	if(0 == tonemap)
		texColor *= exposure;
	else{
		float brightMax = 5.;
		float l = length(texColor);
		float fv = l * exposure;
		texColor *= fv * (fv/brightMax + 1.0) / (fv + 1.0) / l;
//		texColor.xyz = fv / (fv + 1.0);
	}

/*	if(0 == tonemap)
		texColor.rgb *= exposure;
	else{
		float brightMax = 5.;
		vec3 fv = texColor.rgb * exposure;
		texColor.rgb = fv * (fv/brightMax + 1.0) / (fv + 1.0);
//		texColor.xyz = fv / (fv + 1.0);
	}*/
/*	float saturated = 0.;
	for(int i = 0; i < 3; i++)
		saturated = max(0, texColor[i] - 1.);
	// Too bright color saturates the image sensor so that it hides things behind.
	if(1. < saturated){
		texColor.a *= saturated;
	}*/
	return texColor;
}
