// ringshadow.fs

uniform sampler1D tex1d;
uniform sampler2D texshadow;
uniform float ambient;
uniform float ringmin, ringmax;
uniform float sunar;

varying vec3 pos;

void main (void)
{
	if(0. < gl_TexCoord[1][1] - .5){
		float len = length(vec2(gl_TexCoord[1]) - vec2(.5, .5));
		float halfshadow = sunar * (gl_TexCoord[1][1] - .5) / .5;
		gl_FragColor = vec4(len < .5 - halfshadow ? ambient : .5 + halfshadow < len ? 1. : ((1. - ambient) * (len - .5 + halfshadow) / halfshadow / 2. + ambient));
		gl_FragColor[3] = 1.;
	}
	else
		gl_FragColor = vec4(1.);
//	gl_FragColor[3] = texture1D(tex1d, gl_TexCoord[0][0])[3] * .5;
	gl_FragColor *= texture1D(tex1d, (length(pos) - ringmin) / (ringmax - ringmin));
	gl_FragColor[3] *= .5;
}
