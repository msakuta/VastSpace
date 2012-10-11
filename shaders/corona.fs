uniform float exposure;
uniform int tonemap;

varying vec4 col;

void main (void)
{
	vec4 texColor = col;
//	for(int i = 0; i < 4; i++)
//		texColor[i] = (exp(-(1. - col[i]) * (1. - col[i])) - exp(-1.)) / (1. - exp(-1.));

	if(0 == tonemap)
		texColor.xyz *= exposure;
	else{
		float brightMax = 5.;
		texColor[3] *= texColor[3];
		texColor[3] *= texColor[3];
		vec4 fv = texColor * 10. * exposure;
//		for(int i = 3; i < 4; i++)
//			fv[i] = fv[i] * fv[i] * fv[i] * fv[i];
		texColor = fv * (fv/brightMax + 1.0) / (fv + 1.0);
//		texColor.xyz = fv / (fv + 1.0);
	}

	gl_FragColor = texColor;
}
