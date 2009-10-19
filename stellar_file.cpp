#define _CRT_SECURE_NO_WARNINGS

#include "stellar_file.h"
extern "C"{
#include "calc/calc.h"
}
//#include "cmd.h"
//#include "spacewar.h"
#include "astrodef.h"
//#include "astro_star.h"
#include "entity.h"
//#include "island3.h"
//#include "ringworld.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/timemeas.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern double gravityfactor;


struct teleport *tplist = NULL;
int ntplist = 0;


/* circular orbit class */
static void orbitcs_anim(CoordSys *, double dt);
extern void coordsys_draw(CoordSys *, const struct viewer *);


OrbitCS::OrbitCS(const char *path, CoordSys *root){
	OrbitCS *ret = this;
	init(path, root);
	ret->orbit_rad = 0.;
	QUATIDENTITY(ret->orbit_axis);
	ret->orbit_home = NULL;
	ret->orbit_phase = 0.;
	ret->eccentricity = 0.;
	ret->flags2 = 0;
/*	VECNULL(ret->orbit_omg);*/
}

void OrbitCS::anim(double dt){
	int timescale = 0;
	double scale = timescale ? 5000. * pow(10., timescale-1) : 1.;
	double dist;
	double omega;
	Vec3d orbpos, oldpos;
	Vec3d orbit_omg, omgdt;
	orbpos = parent->tocs(orbit_home->pos, orbit_home->parent);
	dist = orbit_rad;
	omega = scale / (dist * sqrt(dist / UGC / (orbit_home->mass)));
	orbit_omg = orbit_axis.norm();
	oldpos = pos;
	if(eccentricity == 0.){
		Quatd rot, q;
		orbit_phase += omega * dt;
		rot[0] = rot[1] = 0.;
		rot[2] = sin(orbit_phase / 2.);
		rot[3] = cos(orbit_phase / 2.);
		q = orbit_axis * rot;
		pos = q.trans(avec3_010);
		pos *= orbit_rad;
	}
	else{
		Vec3d pos0;
		Mat4d rmat, smat, mat;
		double smia; /* semi-minor axis */
		double r;
		pos0[0] = cos(orbit_phase);
		pos0[1] = sin(orbit_phase);
		pos0[2] = 0.;
		smat = mat4_u;
		smia = orbit_rad * sqrt(1. - eccentricity * eccentricity);
		smat.scalein(orbit_rad, smia, orbit_rad);
		smat.translatein(eccentricity, 0, 0);
		rmat = orbit_axis.tomat4();
		mat = rmat * smat;
		pos = mat.vp3(pos0);
		r = pos.len();
		orbit_phase += omega * dt * dist / r;
	}
	pos += orbpos;
	velo = pos - oldpos;
	velo *= 1. / dt;
}

void OrbitCS::draw(const Viewer *vw){
	CoordSys::draw(vw);
/*	Astrobj *a;
	if((CS_EXTENT | CS_ISOLATED) == (flags & (CS_EXTENT | CS_ISOLATED)))
		for(a = aolist; a; a = (Astrobj*)a->next)
			a->draw(vw);*/
}




























/* Lagrange 1 point between two bodies */
Lagrange1CS::Lagrange1CS(const char *path, CoordSys *root){
	init(path, root);
	objs[0] = objs[1] = NULL;
}

void Lagrange1CS::anim(double dt){
	static int init = 0;
	double f, alpha;
	Vec3d delta, oldpos, pos1, pos2;

	if(!objs[0] || !objs[1])
		return;

	alpha = objs[1]->mass / (objs[0]->mass + objs[1]->mass);
	f = (1. - pow(alpha, 1. / 3.));

	pos1 = parent->tocs(objs[0]->pos, objs[0]->parent);
	pos2 = parent->tocs(objs[1]->pos, objs[1]->parent);

	oldpos = pos;
	delta = pos2 - pos1;
	pos = delta * f;
	pos += pos1;
	velo = (pos - oldpos) * (1. / dt);
}

/* Lagrange 2 point between two bodies */
class Lagrange2CS : public CoordSys{
	Astrobj *objs[2];
public:
	Lagrange2CS(const char *path, CoordSys *root);
	void anim(double);
};

Lagrange2CS::Lagrange2CS(const char *path, CoordSys *root){
	init(path, root);
	objs[0] = objs[1] = NULL;
}

void Lagrange2CS::anim(double dt){
	static int init = 0;
	double f, alpha;
	Vec3d delta, oldpos, pos1, pos2;

	if(!objs[0] || !objs[1])
		return;

	alpha = objs[1]->mass / (objs[0]->mass + objs[1]->mass);
	f = (1. - pow(alpha, 1. / 3.));

	pos1 = parent->tocs(objs[0]->pos, objs[0]->parent);
	pos2 = parent->tocs(objs[1]->pos, objs[1]->parent);

	oldpos = pos;
	delta = pos2 - pos1;
	pos = delta * -1.;
	pos += pos1;
	velo = (pos - oldpos) * (1. / dt);
}






























#if 0
static void TexturedPlanet_draw(struct astrobj *a, const struct viewer *vw){}


class TexturedPlanet{
	Astrobj st;
	const char *texname;
};

Astrobj *TexturedPlanet_new(const char *name, CoordSys *cs){
	extern Astrobj **astrobjs;
	extern int nastrobjs;
	TexturedPlanet *p;
	Astrobj *ret;
	astrobjs = realloc(astrobjs, ++nastrobjs * sizeof *astrobjs);
	ret = astrobjs[nastrobjs-1] = &(p = (TexturedPlanet*)malloc(sizeof*p))->st;
	VECNULL(ret->pos);
	QUATIDENTITY(ret->rot);
	ret->cs = cs;
	ret->flags = AO_DRAWVFT;
	ret->vft = TexturedPlanet_draw;
	ret->mass = 100;
	ret->rad = 1.;
	ret->absmag = 0;
	QUATIDENTITY(ret->orbit_axis);
	ret->orbit_phase = 0;
	ret->orbit_radius = 1;
	ret->orbit_home = NULL;
	VECNULL(ret->omg);
	if(name){
		ret->name = malloc(strlen(name) + 1);
		strcpy(ret->name, name);
	}
	else 
		ret->name = "?";
	return ret;
}
#endif









#define MAX_LINE_LENGTH 512

extern "C" int argtok(char *argv[], char *s, char **post, int maxargs);

/* stack state machine */
static int StellarFileLoadInt(const char *fname, CoordSys *root, struct varlist *vl);
static int stellar_astro(StellarContext *sc, Astrobj *a);

static int stellar_coordsys(StellarContext &sc, CoordSys *cs){
	const char *fname = sc.fname;
	const CoordSys *root = sc.root;
	struct varlist *vl = sc.vl;
	int mode = 1;
	int enable = 0;
//	double inclination;
//	double loan; /* Longitude of Ascending Node */
//	double aop; /* Argument of Periapsis */
	while(mode && fgets(sc.buf, MAX_LINE_LENGTH, sc.fp)){
		char *s = NULL, *ps;
		int argc, c = 0;
		char *argv[8], *post;
		sc.line++;
		argc = argtok(argv, sc.buf, &post, numof(argv));
		if(argc == 0)
			continue;
		if(!strcmp("}", argv[argc - 1])){
			argv[--argc] = NULL;
			mode = 0;
			if(argc == 0){
				break;
			}
		}
		if(!strcmp("{", argv[argc - 1])){
			argv[--argc] = NULL;
			if(argc == 0)
				continue;
		}
		s = argv[0];
		ps = argv[1];
		if(!strcmp(s, "define")){
			struct var *v;
			sc.vl->l = (var*)realloc(sc.vl->l, ++sc.vl->c * sizeof *sc.vl->l);
			v = &sc.vl->l[sc.vl->c-1];
			v->name = (char*)malloc(strlen(ps) + 1);
			strcpy(v->name, ps);
			v->type = var::CALC_D;
			v->value.d = 2 < argc ? calc3(const_cast<const char**>(&argv[2]), sc.vl, NULL) : 0.;
			continue;
/*			definition = 2, ps = NULL;*/
		}
		else if(!strcmp(s, "include")){
			StellarFileLoadInt(ps, cs, vl);
			continue;
		}
/*		else if(definition == 2){
			struct var *v;
			v = &sc.vl->l[sc.vl->c-1];
			v->value.d = calc3(&s, calc_mathvars(), NULL);
			s = NULL;
		}*/
		if(!strcmp(argv[0], "new"))
			s = argv[1], ps = argv[2], c++;
/*		if(s[0] == '/')
			cs2 = findcspath(root, s+1);
		else if(cs2 = findcs(root, s));
		else*/ if(!strcmp(s, "astro")){
			Astrobj *a = NULL;
/*			if(ps && !strcmp(ps, "Asteroid")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = asteroid_new;
			}
			else if(ps && !strcmp(ps, "Satellite")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = satellite_new;
			}
			else if(ps && !strcmp(ps, "TextureSphere")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = texsphere_new;
			}
			else if(ps && !strcmp(ps, "Star")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = star_new;
			}
			else if(ps && !strcmp(ps, "Island3")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = island3_new;
			}
			else if(ps && !strcmp(ps, "Iserlohn")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = iserlohn_new;
			}
			else if(ps && !strcmp(ps, "Blackhole")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = blackhole_new;
			}
			else if(ps && !strcmp(ps, "Ringworld")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = RingworldNew;
			}*/
			if(ps){
				char *pp;
				if(pp = strchr(ps, '{'))
					*pp = '\0';
				if((a = findastrobj(ps)) && a->parent == cs)
					stellar_coordsys(sc, a);
				else
					a = NULL;
			}
			if(!a && (a = new Astrobj(ps, cs))){
				stellar_coordsys(sc, a);
			}
		}
		else if(!strcmp(s, "coordsys")){
//			CoordSys *(*constructor)(const char *, CoordSys *) = new_coordsys;
			CoordSys *cs2 = NULL;
/*			if(ps && !strcmp(ps, "Orbit")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = new_orbitcs;
			}
			if(ps && !strcmp(ps, "Lagrange1")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = new_lagrange1;
			}
			if(ps && !strcmp(ps, "Lagrange2")){
				c++, s = argv[c], ps = argv[c+1];
				constructor = new_lagrange2;
			}*/
			if(ps){
				char *pp;
				if(pp = strchr(ps, '{'))
					*pp = '\0';
				if((cs2 = cs->findcspath(ps)))
					stellar_coordsys(sc, cs2);
			}
			if(!cs2 && (cs2 = new CoordSys(ps, cs))){
				stellar_coordsys(sc, cs2);
			}
		}
		else if(cs->readFile(sc, argc, (argv)));
#if 0
		else if(!strcmp(s, "name")){
			if(s = strtok(ps, " \t\r\n")){
				cs->name = malloc(strlen(s) + 1);
				strcpy(cs->name, s);
			}
		}
		else if(!strcmp(s, "fullname")){
			if(s = ps){
				cs->fullname = malloc(strlen(s) + 1);
				strcpy(cs->fullname, s);
			}
		}
		else if(!strcmp(s, "pos")){
			cs->pos[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				cs->pos[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				cs->pos[2] = calc3(&argv[3], vl, NULL);
		}
		else if(!strcmp(s, "radius")){
			if(ps)
				cs->rad = atof(ps);
		}
		else if(!strcmp(s, "parent")){
			CoordSys *cs2, *csret;
			if(ps){
				if(*ps == '/')
					cs2 = root, ps++;
				else
					cs2 = sc->root;
				if((csret = findcspath(cs2, ps)) || (csret = findcs(cs2, ps)))
					adopt_child(cs, csret);
				else
					CmdPrintf("%s(%ld): Unknown CoordSys: %s", sc->fname, sc->line, ps);
			}
/*				if(cs2 = findcs(root, s))
					adopt_child(cs, cs2);
				else
					CmdPrintf("No such CoordSys: %s", s);
			}*/
		}
		else if(!strcmp(s, "solarsystem")){
			/* solar system is by default extent and isolated */
			if(1 < argc){
				if(!strcmp(argv[1], "false"))
					cs->flags &= ~CS_SOLAR;
				else
					cs->flags |= CS_SOLAR | CS_EXTENT | CS_ISOLATED;
			}
			else
				cs->flags |= CS_SOLAR | CS_EXTENT | CS_ISOLATED;
			if(cs->flags & CS_SOLAR){
				extern Astrobj **astrobjorder;
				extern int nastrobjorder;
				astrobjorder = realloc(astrobjorder, (nastrobjorder + 1) * sizeof *astrobjorder);
				astrobjorder[nastrobjorder++] = cs;
			}
		}
		else if(!strcmp(s, "eisystem")){
			/* solar system is by default extent and isolated */
			if(1 < argc){
				if(!strcmp(argv[1], "false"))
					cs->flags &= ~CS_SOLAR;
				else
					cs->flags |= CS_EXTENT | CS_ISOLATED;
			}
			else
				cs->flags |= CS_EXTENT | CS_ISOLATED;
			if(cs->flags & CS_EXTENT){
				extern Astrobj **astrobjorder;
				extern int nastrobjorder;
				astrobjorder = realloc(astrobjorder, (nastrobjorder + 1) * sizeof *astrobjorder);
				astrobjorder[nastrobjorder++] = cs;
			}
		}
		else if(!strcmp(s, "warpable")){
			if(argv[1]){
				if(atoi(argv[1]))
					cs->flags |= CS_WARPABLE;
				else
					cs->flags &= ~CS_WARPABLE;
			}
			else
				cs->flags |= CS_WARPABLE;
		}
		else if(!strcmp(s, "extent")){
			if(argv[1]){
				if(atoi(argv[1]))
					cs->flags |= CS_EXTENT;
				else
					cs->flags &= ~CS_EXTENT;
			}
			else
				cs->flags |= CS_EXTENT;
		}
		else if(!strcmp(s, "isolated")){
			if(argv[1]){
				if(atoi(argv[1]))
					cs->flags |= CS_ISOLATED;
				else
					cs->flags &= ~CS_ISOLATED;
			}
			else
				cs->flags |= CS_ISOLATED;
		}
		else if(!strcmp(s, "teleport") || !strcmp(s, "warp")){
			struct teleport *tp;
			char *name = argc < 2 ? cs->fullname ? cs->fullname : cs->name : argv[1];
			int i;
			for(i = 0; i < ntplist; i++) if(!strcmp(tplist[i].name, name)){
				tplist[i].flags |= !strcmp(s, "teleport") ? TELEPORT_TP : TELEPORT_WARP;
				break;
			}
			if(i != ntplist)
				continue;
			tplist = realloc(tplist, ++ntplist * sizeof *tplist);
			tp = &tplist[ntplist-1];
			tp->cs = cs;
			tp->name = malloc(strlen(name) + 1);
			strcpy(tp->name, name);
			tp->flags = !strcmp(s, "teleport") ? TELEPORT_TP : TELEPORT_WARP;
			tp->pos[0] = 2 < argc ? calc3(&argv[2], vl, NULL) : 0.;
			tp->pos[1] = 3 < argc ? calc3(&argv[3], vl, NULL) : 0.;
			tp->pos[2] = 4 < argc ? calc3(&argv[4], vl, NULL) : 0.;
		}
		else if(!strcmp(s, "rstation")){
			extern struct player *ppl;
			warf_t *w;
			entity_t *pt;
			if(cs->w)
				w = cs->w;
			else
				w = spacewar_create(cs, ppl);
			pt = RstationNew(w);
			pt->pos[0] = 1 < argc ? calc3(&argv[1], vl, NULL) : 0.;
			pt->pos[1] = 2 < argc ? calc3(&argv[2], vl, NULL) : 0.;
			pt->pos[2] = 3 < argc ? calc3(&argv[3], vl, NULL) : 0.;
		}
		else if(!strcmp(s, "addent")){
			extern struct player *ppl;
			warf_t *w;
			entity_t *pt;
			if(cs->w)
				w = cs->w;
			else
				w = spacewar_create(cs, ppl);
			pt = addent_at_warf(w, 2, &argv[0]);
			pt->pos[0] = 2 < argc ? calc3(&argv[2], vl, NULL) : 0.;
			pt->pos[1] = 3 < argc ? calc3(&argv[3], vl, NULL) : 0.;
			pt->pos[2] = 4 < argc ? calc3(&argv[4], vl, NULL) : 0.;
			pt->race = 5 < argc ? atoi(argv[5]) : 0;
		}
		else if(!strcmp(s, "rotation")){
			if(1 < argc)
				cs->qrot[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				cs->qrot[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc){
				cs->qrot[2] = calc3(&argv[3], vl, NULL);
				cs->qrot[3] = sqrt(1. - VECSLEN(cs->qrot));
			}
		}
		else if(!strcmp(s, "updirection")){
			avec3_t v = {0};
			if(1 < argc)
				v[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				v[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				v[2] = calc3(&argv[3], vl, NULL);
			quatdirection(cs->qrot, v);
		}
		else if(!strcmp(s, "omega")){
			if(1 < argc)
				cs->omg[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				cs->omg[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				cs->omg[2] = calc3(&argv[3], vl, NULL);
			if(4 < argc){
				double d;
				VECNORMIN(cs->omg);
				d = calc3(&argv[4], vl, NULL);
				VECSCALEIN(cs->omg, d);
			}
		}
		else if(cs->vft == &orbitcs_s && !strcmp(s, "orbits")){
			orbitcs_t *a = (orbitcs_t *)cs;
			if(s = argv[1])
				a->orbit_home = findastrobj(s);
		}
		else if(cs->vft == &orbitcs_s && (!strcmp(s, "orbit_radius") || !strcmp(s, "semimajor_axis"))){
			orbitcs_t *a = (orbitcs_t *)cs;
			if(s = strtok(ps, " \t\r\n"))
				a->orbit_rad = calc3(&s, vl, NULL);
		}
		else if(cs->vft == &orbitcs_s && !strcmp(s, "orbit_axis")){
			orbitcs_t *a = (orbitcs_t *)cs;
			if(1 < argc)
				a->orbit_axis[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				a->orbit_axis[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc){
				a->orbit_axis[2] = calc3(&argv[3], vl, NULL);
				a->orbit_axis[3] = sqrt(1. - VECSLEN(a->orbit_axis));
			}
		}
		else if(cs->vft == &orbitcs_s && !strcmp(s, "orbit_inclination")){
			orbitcs_t *a = (orbitcs_t *)cs;
			if(1 < argc){
				double d;
				aquat_t q1 = {0,0,0,1};
				avec3_t omg;
				d = calc3(&argv[1], vl, NULL);
				omg[0] = 0.;
				omg[1] = d / deg_per_rad;
				omg[2] = 0.;
				quatrotquat(a->orbit_axis, omg, q1);
			}
		}
		else if((cs->vft == &lagrange1cs_s || cs->vft == &lagrange2cs_s) && !strcmp(s, "object1")){
			lagrange1cs_t *a = (lagrange1cs_t *)cs;
			if(1 < argc){
				double d;
				a->objs[0] = findastrobj(argv[1]);
			}
		}
		else if((cs->vft == &lagrange1cs_s || cs->vft == &lagrange2cs_s) && !strcmp(s, "object2")){
			lagrange1cs_t *a = (lagrange1cs_t *)cs;
			if(1 < argc){
				double d;
				a->objs[1] = findastrobj(argv[1]);
			}
		}
		else if(!strcmp(s, "eccentricity")){
			orbitcs_t *a = (orbitcs_t *)cs;
			if(1 < argc){
				double d;
				d = calc3(&argv[1], vl, NULL);
				a->eccentricity = d;
			}
		}
		else if(!strcmp(s, "inclination")){
			if(1 < argc){
				enable |= 1;
				inclination = calc3(&argv[1], vl, NULL) / deg_per_rad;
			}
		}
		else if(!strcmp(s, "ascending_node")){
			if(1 < argc){
				enable |= 2;
				loan = calc3(&argv[1], vl, NULL) / deg_per_rad;
			}
		}
		else if(!strcmp(s, "argument_of_periapsis")){
			if(1 < argc){
				enable |= 4;
				aop = calc3(&argv[1], vl, NULL) / deg_per_rad;
			}
		}
		else if(!strcmp(s, "showorbit")){
			orbitcs_t *a = (orbitcs_t *)cs;
			if(1 < argc){
				if(!strcmp(argv[1], "false"))
					a->flags2 &= ~OCS_SHOWORBIT;
				else
					a->flags2 |= OCS_SHOWORBIT;
			}
			else
				a->flags2 |= OCS_SHOWORBIT;
		}
#endif
		else if(s = strtok(s, " \t\r\n")){
//			CmdPrintf("%s(%ld): Unknown parameter for CoordSys: %s", sc.fname, sc.line, s);
			printf("%s(%ld): Unknown parameter for %s: %s\n", sc.fname, sc.line, cs->classname(), s);
		}
	}
	return mode;
}

#if 0
static int stellar_astro(StellarContext *sc, Astrobj *a){
	const char *fname = sc->fname;
	const CoordSys *root = sc->root;
	struct varlist *vl = sc->vl;
	int mode = 1;
	int enable = 0;
	double inclination;
	double loan; /* Longitude of Ascending Node */
	double aop; /* Argument of Periapsis */
	while(fgets(sc->buf, MAX_LINE_LENGTH, sc->fp)){
		char *s = NULL, *ps;
		int i, argc;
		char *argv[8], *post;
		sc->line++;
		argc = argtok(argv, sc->buf, &post, numof(argv));
		if(argc == 0)
			continue;
		if(!strcmp("}", argv[argc - 1])){
			argv[--argc] = NULL;
			mode = 0;
			if(argc == 0){
				break;
			}
		}
		if(!strcmp("{", argv[argc - 1])){
			argv[--argc] = NULL;
			if(argc == 0)
				continue;
		}
		s = argv[0];
		ps = argv[1];
		if(!strcmp(s, "name")){
			if(s = strtok(ps, " \t\r\n")){
				a->name = malloc(strlen(s) + 1);
				strcpy(a->name, s);
			}
		}
		else if(!strcmp(s, "pos")){
			a->pos[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				a->pos[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				a->pos[2] = calc3(&argv[3], vl, NULL);
		}
		else if(!strcmp(s, "diameter")){
			if(argv[1])
				a->rad = .5 * calc3(&argv[1], vl, NULL);
		}
		else if(!strcmp(s, "radius")){
			if(argv[1]){
				a->rad = calc3(&argv[1], vl, NULL);
				if(a->flags & AO_BLACKHOLE)
					a->mass = LIGHT_SPEED * LIGHT_SPEED / (2 * UGC * a->rad);
			}
		}
		else if(!strcmp(s, "mass")){
			if(argv[1]){
				a->mass = calc3(&argv[1], vl, NULL);
				if(a->flags & AO_BLACKHOLE)
					a->rad = 2 * UGC * a->mass / LIGHT_SPEED / LIGHT_SPEED;
			}
		}
		else if(!strcmp(s, "gravity")){
			if(1 < argc){
				if(atoi(argv[1]))
					a->flags |= AO_GRAVITY;
				else
					a->flags &= ~AO_GRAVITY;
			}
			else
				a->flags |= AO_GRAVITY;
		}
		else if(!strcmp(s, "spherehit")){
			if(1 < argc){
				if(atoi(argv[1]))
					a->flags |= AO_SPHEREHIT;
				else
					a->flags &= ~AO_SPHEREHIT;
			}
			else
				a->flags |= AO_SPHEREHIT;
		}
		else if(!strcmp(s, "planet")){
			if(1 < argc){
				if(atoi(argv[1]))
					a->flags |= AO_PLANET;
				else
					a->flags &= ~AO_PLANET;
			}
			else
				a->flags |= AO_PLANET;
		}
		else if(!strcmp(s, "color")){
			if(s = strtok(ps, " \t\r\n")){
				COLOR32 c;
				c = strtoul(s, NULL, 16);
				a->basecolor = COLOR32RGBA(COLOR32B(c),COLOR32G(c),COLOR32R(c),255);
			}
		}
		else if(!strcmp(s, "orbits")){
			if(s = argv[1])
				a->orbit_home = findastrobj(s);
		}
		else if((!strcmp(s, "orbit_radius") || !strcmp(s, "semimajor_axis"))){
			if(s = strtok(ps, " \t\r\n"))
				a->orbit_radius = calc3(&s, vl, NULL);
		}
		else if(!strcmp(s, "orbit_axis")){
			if(1 < argc)
				a->orbit_axis[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				a->orbit_axis[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc){
				a->orbit_axis[2] = calc3(&argv[3], vl, NULL);
				a->orbit_axis[3] = sqrt(1. - VECSLEN(a->orbit_axis));
			}
		}
		else if(!strcmp(s, "orbit_inclination")){
			if(1 < argc){
				double d;
				aquat_t q1 = {0,0,0,1};
				avec3_t omg;
				d = calc3(&argv[1], vl, NULL);
				omg[0] = 0.;
				omg[1] = d / deg_per_rad;
				omg[2] = 0.;
				quatrotquat(a->orbit_axis, omg, q1);
			}
		}
		else if(!strcmp(s, "eccentricity")){
			if(1 < argc){
				double d;
				d = calc3(&argv[1], vl, NULL);
				a->flags |= AO_ELLIPTICAL;
				a->eccentricity = d;
			}
		}
		else if(!strcmp(s, "rotation")){
			if(1 < argc)
				a->rot[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				a->rot[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc){
				a->rot[2] = calc3(&argv[3], vl, NULL);
				a->rot[3] = sqrt(1. - VECSLEN(a->orbit_axis));
			}
		}
		else if(!strcmp(s, "updirection")){
			avec3_t v = {0};
			if(1 < argc)
				v[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				v[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				v[2] = calc3(&argv[3], vl, NULL);
			quatdirection(a->rot, v);
		}
		else if(!strcmp(s, "CoordSys")){
			CoordSys *cs2;
			if(ps){
				CoordSys *cs;
				if(*ps == '/')
					cs = root, ps++;
				else
					cs = sc->root;
				if((cs2 = findcspath(cs, ps)) || (cs2 = findcs(cs, ps)))
					a->cs = cs2;
				else
					CmdPrintf("%s(%ld): Unknown CoordSys: %s", sc->fname, sc->line, ps);
			}
		}
		else if(!strcmp(s, "omega")){
			if(1 < argc)
				a->omg[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				a->omg[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				a->omg[2] = calc3(&argv[3], vl, NULL);
			if(4 < argc){
				double d;
				VECNORMIN(a->omg);
				d = calc3(&argv[4], vl, NULL);
				VECSCALEIN(a->omg, d);
			}
		}
/*		else if(!strcmp(s, "drawmethod")){
			CoordSys *cs2;
			if((s = strtok(ps, " \t\r\n"))){
				if(!strcmp(s, "star")){
					a->vft = sun_draw;
					a->flags |= AO_DRAWVFT | AO_STARGLOW;
				}
				else if(!strcmp(s, "ringplanet")){
					extern const struct astrobj_static ringplanet_s;
					a->vft = &ringplanet_s;
					a->flags &= ~AO_DRAWVFT;
				}*/
/*				else if(!strcmp(s, "jupiter")){
					extern const struct astrobj_static jupiter_s;
					a->vft = &jupiter_s;
					a->flags &= ~AO_DRAWVFT;
				}*/
/*			}
		}*/
		else if(!strcmp(s, "inclination")){
			if(1 < argc){
				enable |= 1;
				inclination = calc3(&argv[1], vl, NULL) / deg_per_rad;
			}
		}
		else if(!strcmp(s, "ascending_node")){
			if(1 < argc){
				enable |= 2;
				loan = calc3(&argv[1], vl, NULL) / deg_per_rad;
			}
		}
		else if(!strcmp(s, "argument_of_periapsis")){
			if(1 < argc){
				enable |= 4;
				aop = calc3(&argv[1], vl, NULL) / deg_per_rad;
			}
		}
		else if(&texsphere_s == a->vft && !strcmp(s, "texture")){
			texsphere *p = (texsphere*)a;
			if(1 < argc){
				p->texname = malloc(strlen(argv[1]) + 1);
				strcpy(p->texname, argv[1]);
			}
		}
		else if(&texsphere_s == a->vft && !strcmp(s, "atmosphere_height")){
			texsphere *p = (texsphere*)a;
			if(1 < argc){
				p->atmodensity = calc3(&argv[1], vl, NULL);
				a->flags |= AO_ATMOSPHERE;
			}
		}
		else if(&texsphere_s == a->vft && !strcmp(s, "atmosphere_color")){
			texsphere *p = (texsphere*)a;
			if(1 < argc)
				p->atmohor[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				p->atmohor[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				p->atmohor[2] = calc3(&argv[3], vl, NULL);
			if(4 < argc)
				p->atmohor[3] = calc3(&argv[4], vl, NULL);
			else
				p->atmohor[3] = 1.f;
		}
		else if(&texsphere_s == a->vft && !strcmp(s, "atmosphere_dawn")){
			texsphere *p = (texsphere*)a;
			if(1 < argc)
				p->atmodawn[0] = calc3(&argv[1], vl, NULL);
			if(2 < argc)
				p->atmodawn[1] = calc3(&argv[2], vl, NULL);
			if(3 < argc)
				p->atmodawn[2] = calc3(&argv[3], vl, NULL);
			if(4 < argc)
				p->atmodawn[3] = calc3(&argv[4], vl, NULL);
			else
				p->atmodawn[3] = 1.f;
		}
		else if(a->vft == &texsphere_s && (!strcmp(s, "ringmin") || !strcmp(s, "ringmax") || !strcmp(s, "ringthick"))){
			texsphere *p = (texsphere*)a;
			p->ring = 1;
			*(!strcmp(s, "ringmin") ? &p->ringmin : !strcmp(s, "ringmax") ? &p->ringmax : &p->ringthick) = calc3(&ps, vl, NULL);
		}
		else if(!strcmp(s, "absolute_magnitude")){
			a->absmag = calc3(&ps, vl, NULL);
		}
		else if(s = strtok(s, " \t\r\n")){
			CmdPrintf("%s(%ld): Unknown parameter for Astro: %s", sc->fname, sc->line, s);
		}
	}
	if(a->flags & AO_PLANET && enable == 7){
		if(inclination == 0.)
			QUATIDENTITY(a->orbit_axis);
		else{
			aquat_t q1, q2, q3, q4;
			q1[0] = sin(inclination / 2.);
			q1[1] = 0.;
			q1[2] = 0.;
			q1[3] = cos(inclination / 2.);
			q2[0] = 0.;
			q2[1] = 0.;
			q2[2] = sin(loan / 2.);
			q2[3] = cos(loan / 2.);
			QUATMUL(q4, q2, q1);
			q3[0] = 0.;
			q3[1] = 0.;
			q3[2] = sin(aop / 2.);
			q3[3] = cos(aop / 2.);
			QUATMUL(a->orbit_axis, q4, q3);
		}
	}
	return mode;
}
#endif

static int StellarFileLoadInt(const char *fname, CoordSys *root, struct varlist *vl){
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
		FILE *fp;
		char buf[MAX_LINE_LENGTH];
		int mode = 0;
		int inquote = 0;
		StellarContext sc;
		CoordSys *cs = NULL;
		Astrobj *a = NULL;
		sc.fname = fname;
		sc.root = root;
		sc.line = 0;
		sc.fp = fp = fopen(fname, "r");
		if(!fp)
			return -1;
		sc.buf = buf;
		sc.vl = (varlist*)malloc(sizeof *sc.vl);
		sc.vl->c = 0;
		sc.vl->l = NULL;
		sc.vl->next = vl;

		stellar_coordsys(sc, root);

/*		CmdPrint("space.dat loaded.");*/
		fclose(fp);
		if(sc.vl->l)
			free(sc.vl->l);
		free(sc.vl);
//		CmdPrintf("%s loaded time %lg", fname, TimeMeasLap(&tm));
		printf("%s loaded time %lg\n", fname, TimeMeasLap(&tm));
	}
	return 0;
}

int StellarFileLoad(const char *fname, CoordSys *root){
	return StellarFileLoadInt(fname, root, const_cast<varlist*>(calc_mathvars()));
}
