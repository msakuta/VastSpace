// ringshadow.fs

uniform sampler1D texRing, texRingBack;
uniform sampler2D texshadow;
uniform float ambient;
uniform float ringmin, ringmax;
uniform float sunar;
uniform float backface;
uniform float exposure;

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
	float f = backface * max(0., min(1., 1. + gl_TexCoord[2][1] / 2.));
	float coord = (length(pos) - ringmin) / (ringmax - ringmin);
	vec4 ringcol = (1. - f) * texture1D(texRing, coord) + .5 * f * texture1D(texRingBack, coord);
	ringcol[3] = texture1D(texRing, coord)[3];
	gl_FragColor *= ringcol;
	float alpha = gl_FragColor[3] + max(0., exposure * (gl_FragColor[0] + gl_FragColor[1] + gl_FragColor[2]) / 3. - 1.);
	gl_FragColor *= exposure;
	gl_FragColor[3] = alpha;
}
