// ringshadow.fs

uniform sampler1D tex1d;
uniform sampler2D texshadow;
uniform float ambient;
uniform float ringmin, ringmax;

varying vec3 pos;

void main (void)
{
	if(0. < gl_TexCoord[1][1] - .5){
		float len = length(vec2(gl_TexCoord[1]) - vec2(.5, .5));
		gl_FragColor = vec4(len < .495 ? ambient : .505 < len ? 1. : ((1. - ambient) * (len - .495) / .01 + ambient));
	}
	else
		gl_FragColor = vec4(1.);
//	gl_FragColor[3] = texture1D(tex1d, gl_TexCoord[0][0])[3] * .5;
	gl_FragColor[3] = texture1D(tex1d, (length(pos) - ringmin) / (ringmax - ringmin))[3] * .5;
}
