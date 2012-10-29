varying vec4 col;

void main(void)
{
	col = gl_Color;
	gl_Position = ftransform();
}
