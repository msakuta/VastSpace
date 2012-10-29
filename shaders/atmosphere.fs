#include "shaders/tonemap.fs"

varying vec4 col;

void main (void)
{
	gl_FragColor = toneMappingAlpha(col);
}
