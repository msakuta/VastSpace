#ifndef DRAW_RING_DRAW_H
#define DRAW_RING_DRAW_H
#include "../astro.h"
#include "../viewer.h"

#define RING_CUTS 64

class AstroRing{
	GLuint ringTex, ringBackTex, ringShadowTex;
	GLint tex1dLoc, texLoc, ambientLoc, ringminLoc, ringmaxLoc, ringnormLoc, exposureLoc;
	bool shader_compile;
	GLuint ring_setshadow(double angle, double ipitch, double minrad, double maxrad, double sunar, float backface, float exposure);
public:
	AstroRing() : ringTex(0), ringBackTex(0), ringShadowTex(0), shader_compile(false){}
	void ring_draw(const Viewer &vw, const Astrobj *a, const Vec3d &sunpos, char start, char end, const Quatd &rotation, double thick,
			   double minrad, double maxrad, double t, double oblateness, const char *ringTexName = NULL, const char *rinBackTexName = NULL,
			   double sunar = .01);
	void ring_setsphereshadow(const Viewer &vw, double minrad, double maxrad, const Vec3d &ringnorm, GLuint program);
	void ring_setsphereshadow_end();
};

#endif
