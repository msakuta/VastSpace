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
		vec3 fv = texColor.xyz * exposure;
		texColor.xyz = fv * (fv/brightMax + 1.0) / (fv + 1.0);
//		texColor.xyz = fv / (fv + 1.0);
	}
	return texColor;
}

vec4 toneMappingAlpha(vec4 texColor)
{
	if(0 == tonemap)
		texColor *= exposure;
	else{
		float brightMax = 5.;
		vec4 fv = texColor * exposure;
		texColor = fv * (fv/brightMax + 1.0) / (fv + 1.0);
//		texColor.xyz = fv / (fv + 1.0);
	}
	return texColor;
}
