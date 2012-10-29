#include "shaders/tonemap.fs"

varying vec4 col;

/// Atmosphere has a special logic distinct from toneMappingAlpha().
vec4 toneMappingAtmosphere(vec4 texColor)
{
	if(0 == tonemap)
		texColor *= exposure;
	else{
		float l = length(texColor);
		float fv = l * exposure;
		texColor *= fv * (fv/brightMax + 1.0) / (fv + 1.0) / l;
//		texColor.xyz = fv / (fv + 1.0);
	}
	return texColor;
}

void main (void)
{
	gl_FragColor = toneMappingAtmosphere(col);
}
