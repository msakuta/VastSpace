#include "astro.h"
#include "serial_util.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "astrodef.h"
#include "player.h"
#include "cmd.h"
extern "C"{
#include "calc/calc.h"
#include <clib/mathdef.h>
}
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>


double OrbitCS::astro_timescale = 1.;


const char *OrbitCS::classname()const{
	return "Orbit";
}

OrbitCS::OrbitCS(const char *path, CoordSys *root) : st(path, root){
	OrbitCS *ret = this;
//	init(path, root);
	ret->orbit_rad = 0.;
	QUATIDENTITY(ret->orbit_axis);
	ret->orbit_home = NULL;
	ret->orbit_phase = 0.;
	ret->eccentricity = 0.;
	ret->flags2 = 0;
/*	VECNULL(ret->orbit_omg);*/
	CoordSys *eis = findeisystem();
	if(eis)
		eis->addToDrawList(this);
}

const ClassRegister<OrbitCS> OrbitCS::classRegister("Orbit");

void OrbitCS::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << orbit_rad << orbit_home << orbit_axis;
	sc.o << orbit_phase;
	sc.o << eccentricity; /* orbital element */
	sc.o << flags2;
}

void OrbitCS::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> orbit_rad >> orbit_home >> orbit_axis;
	sc.i >> orbit_phase;
	sc.i >> eccentricity; /* orbital element */
	sc.i >> flags2;
}

void OrbitCS::anim(double dt){
	double scale = astro_timescale/*timescale ? 5000. * pow(10., timescale-1) : 1.*/;
	if(orbit_home){
		int timescale = 0;
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
			pos = q.trans(vec3_010);
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
		if(dt){
			velo = pos - oldpos;
			velo *= 1. / dt;
		}
	}
	rot = rot.quatrotquat(omg * dt * astro_timescale);
	st::anim(dt);
}

bool OrbitCS::readFileStart(StellarContext &){
	enable = 0;
	return true;
}

bool OrbitCS::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "orbits")){
		if(argv[1]){
			CoordSys *cs = parent->findcspath(ps);
			if(cs)
				orbit_home = cs->toAstrobj();
		}
		return true;
	}
	else if(!strcmp(s, "orbit_radius") || !strcmp(s, "semimajor_axis")){
		if(s = strtok(ps, " \t\r\n"))
			orbit_rad = calc3(&s, sc.vl, NULL);
		return true;
	}
	else if(!strcmp(s, "eccentricity")){
		if(1 < argc){
			double d;
			d = calc3(&argv[1], sc.vl, NULL);
			eccentricity = d;
		}
		return true;
	}
	else if(!strcmp(s, "orbit_axis")){
		if(1 < argc)
			orbit_axis[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			orbit_axis[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc){
			orbit_axis[2] = calc3(&argv[3], sc.vl, NULL);
			orbit_axis[3] = sqrt(1. - VECSLEN(orbit_axis));
		}
		return true;
	}
	else if(!strcmp(s, "orbit_inclination") || !strcmp(s, "inclination")){
		if(1 < argc){
			enable |= 1;
			inclination = calc3(&argv[1], sc.vl, NULL) / deg_per_rad;
		}
/*		if(1 < argc){
			double d;
			Quatd q1(0,0,0,1);
			avec3_t omg;
			d = calc3(&argv[1], sc.vl, NULL);
			omg[0] = 0.;
			omg[1] = d / deg_per_rad;
			omg[2] = 0.;
			orbit_axis = q1.quatrotquat(omg);
		}*/
		return true;
	}
	else if(!strcmp(s, "ascending_node")){
		if(1 < argc){
			enable |= 2;
			loan = calc3(&argv[1], sc.vl, NULL) / deg_per_rad;
		}
		return true;
	}
	else if(!strcmp(s, "argument_of_periapsis")){
		if(1 < argc){
			enable |= 4;
			aop = calc3(&argv[1], sc.vl, NULL) / deg_per_rad;
		}
		return true;
	}
	else if(!strcmp(s, "showorbit")){
		if(1 < argc){
			if(!strcmp(argv[1], "false"))
				flags2 &= ~OCS_SHOWORBIT;
			else
				flags2 |= OCS_SHOWORBIT;
		}
		else
			flags2 |= OCS_SHOWORBIT;
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
	return true;
}

bool OrbitCS::readFileEnd(StellarContext &sc){
	if(!st::readFileEnd(sc))
		return false;
	if(!enable)
		return true;
	if(inclination == 0.)
		orbit_axis = quat_u;
	else{
		Quatd q1, q2, q3, q4;
		q1[0] = sin(inclination / 2.);
		q1[1] = 0.;
		q1[2] = 0.;
		q1[3] = cos(inclination / 2.);
		q2[0] = 0.;
		q2[1] = 0.;
		q2[2] = sin(loan / 2.);
		q2[3] = cos(loan / 2.);
		q4 = q2 * q1;
		if(enable & 4){
			q3[0] = 0.;
			q3[1] = 0.;
			q3[2] = sin(aop / 2.);
			q3[3] = cos(aop / 2.);
			orbit_axis = q4 * q3;
		}
		else
			orbit_axis = q4;
	}
	return true;
}

OrbitCS *OrbitCS::toOrbitCS(){
	return this;
}

Astrobj::Astrobj(const char *name, CoordSys *cs) : st(name, cs), mass(1e10), absmag(30), basecolor(.5f,.5f,.5f,1.){
}

const ClassRegister<Astrobj> Astrobj::classRegister("Astrobj");

#if 0
	double rad;
	double mass;
	float absmag; /* Absolute Magnitude */
	COLOR32 basecolor; /* rough approximation of apparent color */
#endif

void Astrobj::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << " " << rad;
	sc.o << " " << mass;
	sc.o << " " << absmag;
	sc.o << " " << basecolor;
}

void Astrobj::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> " " >> rad;
	sc.i >> " " >> mass;
	sc.i >> " " >> absmag;
	sc.i >> " " >> basecolor;
}

bool Astrobj::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "radius")){
		if(argv[1]){
			rad = calc3(&argv[1], sc.vl, NULL);
		}
		return true;
	}
	else if(!strcmp(s, "diameter")){
		if(argv[1])
			rad = .5 * calc3(&argv[1], sc.vl, NULL);
		return true;
	}
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
			basecolor = Vec4f(COLOR32B(c)/256.f,COLOR32G(c)/256.f,COLOR32R(c)/256.f,1.);
		}
	}
	else if(!strcmp(s, "basecolor")){
		if(3 < argc){
			basecolor = Vec4f(
				GLfloat(calc3(&argv[1], sc.vl, NULL)),
				GLfloat(calc3(&argv[2], sc.vl, NULL)),
				GLfloat(calc3(&argv[3], sc.vl, NULL)),
				1.f);
		}
		return true;
	}
	else if(!strcmp(s, "absolute_magnitude")){
		absmag = GLfloat(calc3(&ps, sc.vl, NULL));
	}
	else
		return st::readFile(sc, argc, argv);
	return true;
}

bool Astrobj::readFileEnd(StellarContext &sc){
	if(!st::readFileEnd(sc))
		return false;
	if((flags & (CS_ISOLATED | CS_EXTENT)) == (CS_ISOLATED | CS_EXTENT)){
		AOList::iterator i;
		for(i = aorder.begin(); i != aorder.end(); i++) if(*i == this)
			break;
		if(i == aorder.end()){
			aorder.push_back(this);
		}
	}
	return true;
}

#if 0
	const char *texname;
	unsigned int texlist; // should not really be here
	double ringmin, ringmax, ringthick;
	double atmodensity;
	float atmohor[4];
	float atmodawn[4];
	int ring;
#endif

const char *Astrobj::classname()const{
	return "Astrobj";
}

/** Finds an Astrobj nearest to this node.
 */
Astrobj *CoordSys::findastrobj(const char *name){
//	int i;
	if(!name)
		return NULL;
	CoordSys *cs;
/*	for(cs = children; cs; cs = cs->next){
		Astrobj *ret = cs->toAstrobj();
		if(ret && !strcmp(ret->name, name))
			return ret;
	}*/
	if(!strchr(name, '/')){
		cs = findcs(name);
	}
	else
		cs = findcspath(name);
	return cs ? cs->toAstrobj() : NULL;
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
		if(a && a->absmag < 30){
			double sd = (retcs->pos - retcs->tocs(vec3_000, a)).slen();
			val = 0. < sd ? pow(2.512, -1.*a->absmag) / sd : 0.;
		}
		else
			val = 0.;
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

	v1 = cs->rot.trans(src);
/*	MAT4VP3(v1, cs->rot, src);*/
	v = v1 + cs->pos;

	double val;
	Astrobj *a = cs2->toAstrobj()/*dynamic_cast<Astrobj*>(cs2)*/;
	val = a && a->absmag < 30 ? pow(2.512, -1.*a->absmag) / (retcs->pos - retcs->tocs(vec3_000, a)).slen() : 0.;
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


Astrobj *CoordSys::findBrightest(const Vec3d &pos){
	Param p = {NULL, 0.};
	findchildbr(p, this, pos, this, NULL);
	findparentbr(p, this, pos, this);
	return p.ret;
}


Star::Star(const char *name, CoordSys *cs) : Astrobj(name, cs){ absmag = 0; }

const char *Star::classname()const{
	return "Star";
}

const ClassRegister<Star> Star::classRegister("Star");


class Surface : public CoordSys{
	Astrobj *a;
	Vec3d offset;
public:
	typedef CoordSys st;
	Surface(){}
	Surface(const char *path, CoordSys *parent) : st(path, parent), a(NULL), offset(0,0,0){}
	Surface(Astrobj *aa) : st(aa->getpath(), aa), a(aa){}
	virtual void anim(double dt){
	}
};
