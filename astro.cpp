#include "astro.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "astrodef.h"
extern "C"{
#include "calc/calc.h"
}
#include <string.h>
#include <stdlib.h>

Astrobj **astrobjs;
int nastrobjs;

Astrobj::Astrobj(const char *name, CoordSys *cs) : CoordSys(name, cs), absmag(-10){
	CoordSys *eis = findeisystem();
	basecolor = COLOR32RGBA(127,127,127,255);
	if(eis){
		eis->aorder = (CoordSys**)realloc(eis->aorder, (eis->naorder + 1) * sizeof *eis->aorder);
		eis->aorder[eis->naorder++] = this;
	}
}

bool Astrobj::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "mass")){
		if(argv[1]){
			mass = calc3(&argv[1], sc.vl, NULL);
			if(flags & AO_BLACKHOLE)
				rad = 2 * UGC * mass / LIGHT_SPEED / LIGHT_SPEED;
		}
		return true;
	}
	else if(!strcmp(s, "gravity")){
		if(1 < argc){
			if(atoi(argv[1]))
				flags |= AO_GRAVITY;
			else
				flags &= ~AO_GRAVITY;
		}
		else
			flags |= AO_GRAVITY;
	}
	else if(!strcmp(s, "spherehit")){
		if(1 < argc){
			if(atoi(argv[1]))
				flags |= AO_SPHEREHIT;
			else
				flags &= ~AO_SPHEREHIT;
		}
		else
			flags |= AO_SPHEREHIT;
	}
	else if(!strcmp(s, "planet")){
		if(1 < argc){
			if(atoi(argv[1]))
				flags |= AO_PLANET;
			else
				flags &= ~AO_PLANET;
		}
		else
			flags |= AO_PLANET;
	}
	else if(!strcmp(s, "color")){
		if(s = strtok(ps, " \t\r\n")){
			COLOR32 c;
			c = strtoul(s, NULL, 16);
			basecolor = COLOR32RGBA(COLOR32B(c),COLOR32G(c),COLOR32R(c),255);
		}
	}
	else if(!strcmp(s, "absolute_magnitude")){
		absmag = calc3(&ps, sc.vl, NULL);
	}
	else
		return CoordSys::readFile(sc, argc, argv);
	return true;
}

bool TexSphere::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "texture")){
		if(1 < argc){
			char *texname = new char[strlen(argv[1]) + 1];
			strcpy(texname, argv[1]);
			this->texname = texname;
		}
		return true;
	}
	else if(!strcmp(s, "atmosphere_height")){
		if(1 < argc){
			atmodensity = calc3(&ps, sc.vl, NULL);
			flags |= AO_ATMOSPHERE;
		}
		return true;
	}
	else if(!strcmp(s, "atmosphere_color")){
		if(1 < argc)
			atmohor[0] = float(calc3(&argv[1], sc.vl, NULL));
		if(2 < argc)
			atmohor[1] = float(calc3(&argv[2], sc.vl, NULL));
		if(3 < argc)
			atmohor[2] = float(calc3(&argv[3], sc.vl, NULL));
		if(4 < argc)
			atmohor[3] = float(calc3(&argv[4], sc.vl, NULL));
		else
			atmohor[3] = 1.f;
		return true;
	}
	else if(!strcmp(s, "atmosphere_dawn")){
		if(1 < argc)
			atmodawn[0] = float(calc3(&argv[1], sc.vl, NULL));
		if(2 < argc)
			atmodawn[1] = float(calc3(&argv[2], sc.vl, NULL));
		if(3 < argc)
			atmodawn[2] = float(calc3(&argv[3], sc.vl, NULL));
		if(4 < argc)
			atmodawn[3] = float(calc3(&argv[4], sc.vl, NULL));
		else
			atmodawn[3] = 1.f;
		return true;
	}
	else
		return CoordSys::readFile(sc, argc, argv);
}

const char *Astrobj::classname()const{
	return "Astrobj";
}

Astrobj *findastrobj(const char *name){
	int i;
	if(!name)
		return NULL;
	for(i = nastrobjs-1; 0 <= i; i--) if(!strcmp(astrobjs[i]->name, name)){
		return astrobjs[i];
	}
	return NULL;
}


static int tocs_children_invokes = 0;

struct Param{
	Astrobj *ret;
	double brightness;
};

static int findchildbr(Param &p, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, const CoordSys *skipcs){
	double best = 0;
#if 1
	CoordSys *cs2;
	tocs_children_invokes++;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
#else
	int i;
	for(i = 0; i < cs->nchildren; i++) if(cs->children[i] && cs->children[i] != skipcs)
	{
		const coordsys *cs2 = cs->children[i];
#endif
		double val;
		Astrobj *a = cs2->toAstrobj()/*dynamic_cast<Astrobj*>(cs2)*/;
		val = a ? pow(2.512, -1.*a->absmag) / (retcs->pos - retcs->tocs(vec3_000, a)).slen() : 0.;
		if(p.brightness < val){
			p.brightness = val;
			p.ret = a;
		}
		if(findchildbr(p, retcs, src, cs2, NULL))
			return 1;
	}
	return 0;
}

static int findparentbr(Param &p, const CoordSys *retcs, const Vec3d &src, CoordSys *cs){
	Vec3d v1, v;
	CoordSys *cs2 = cs->parent;

	if(!cs->parent){
		return 0;
	}

	v1 = cs->qrot.trans(src);
/*	MAT4VP3(v1, cs->rot, src);*/
	v = v1 + cs->pos;

	double val;
	Astrobj *a = cs2->toAstrobj()/*dynamic_cast<Astrobj*>(cs2)*/;
	val = a ? pow(2.512, -1.*a->absmag) / (retcs->pos - retcs->tocs(vec3_000, a)).slen() : 0.;
	if(p.brightness < val){
		p.brightness = val;
		p.ret = a;
	}
	if(p.brightness < val)
		p.brightness = val;

	/* do not scan subtrees already checked! */
	if(findchildbr(p, retcs, src, cs->parent, cs))
		return 1;

	return findparentbr(p, retcs, src, cs->parent);
}


Astrobj *Astrobj::findBrightest()const{
	Param p = {NULL, 0.};
	findchildbr(p, this, vec3_000, this, NULL);
	findparentbr(p, this, vec3_000, const_cast<Astrobj*>(this));
	return p.ret;
}


Star::Star(const char *name, CoordSys *cs) : Astrobj(name, cs){ absmag = 0; }

const char *Star::classname()const{
	return "Star";
}

TexSphere::TexSphere(const char *name, CoordSys *cs) : st(name, cs){
	texlist = 0;
	ringmin = ringmax = 0;
	atmodensity = 0.;
	ring = 0;
}

const char *TexSphere::classname()const{
	return "TexSphere";
}
