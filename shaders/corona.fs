#include "shaders/tonemap.fs"

varying vec4 col;

void main (void)
{
	vec4 texColor = col;
//	for(int i = 0; i < 4; i++)
//		texColor[i] = (exp(-(1. - col[i]) * (1. - col[i])) - exp(-1.)) / (1. - exp(-1.));

	gl_FragColor = toneMapping(texColor);
}
