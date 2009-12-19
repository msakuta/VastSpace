#include "astro.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "astrodef.h"
extern "C"{
#include "calc/calc.h"
#include <clib/mathdef.h>
}
#include <string.h>
#include <stdlib.h>



const char *OrbitCS::classname()const{
	return "OrbitCS";
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

const unsigned OrbitCS::classid = registerClass("OrbitCS", Conster<OrbitCS>);

void OrbitCS::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void OrbitCS::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

void OrbitCS::anim(double dt){
	if(orbit_home){
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
		velo = pos - oldpos;
		velo *= 1. / dt;
	}
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
		QUATIDENTITY(orbit_axis);
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
		q3[0] = 0.;
		q3[1] = 0.;
		q3[2] = sin(aop / 2.);
		q3[3] = cos(aop / 2.);
		orbit_axis = q4 * q3;
	}
	return true;
}

OrbitCS *OrbitCS::toOrbitCS(){
	return this;
}

Astrobj::Astrobj(const char *name, CoordSys *cs) : st(name, cs), mass(1e10), absmag(30), basecolor(COLOR32RGBA(127,127,127,255)){
}

const unsigned Astrobj::classid = registerClass("Astrobj", Conster<Astrobj>);

void Astrobj::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void Astrobj::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
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
			basecolor = COLOR32RGBA(COLOR32B(c),COLOR32G(c),COLOR32R(c),255);
		}
	}
	else if(!strcmp(s, "absolute_magnitude")){
		absmag = calc3(&ps, sc.vl, NULL);
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
	else if((!strcmp(s, "ringmin") || !strcmp(s, "ringmax") || !strcmp(s, "ringthick"))){
		TexSphere *const p = this;
		p->ring = 1;
		*(!strcmp(s, "ringmin") ? &p->ringmin : !strcmp(s, "ringmax") ? &p->ringmax : &p->ringthick) = calc3(&ps, sc.vl, NULL);
	}
	else
		return st::readFile(sc, argc, argv);
}

const char *Astrobj::classname()const{
	return "Astrobj";
}

Astrobj *CoordSys::findastrobj(const char *name){
//	int i;
	if(!name)
		return NULL;
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next){
		Astrobj *ret = cs->toAstrobj();
		if(ret && !strcmp(ret->name, name))
			return ret;
	}
/*	for(i = nastrobjs-1; 0 <= i; i--) if(!strcmp(astrobjs[i]->name, name)){
		return astrobjs[i];
	}*/
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
		val = a && a->absmag < 30 ? pow(2.512, -1.*a->absmag) / (retcs->pos - retcs->tocs(vec3_000, a)).slen() : 0.;
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

const unsigned Star::classid = registerClass("Star", Conster<Star>);

TexSphere::TexSphere(const char *name, CoordSys *cs) : st(name, cs), texname(NULL){
	texlist = 0;
	ringmin = ringmax = 0;
	atmodensity = 0.;
	ring = 0;
}

const char *TexSphere::classname()const{
	return "TexSphere";
}

const unsigned TexSphere::classid = registerClass("TexSphere", Conster<TexSphere>);

double TexSphere::atmoScatter(const Viewer &vw)const{
	if(!(flags & AO_ATMOSPHERE))
		return st::atmoScatter(vw);
	double dist = const_cast<TexSphere*>(this)->calcDist(vw);
	double thick = atmodensity;
	double d = (dist - rad) < thick * 128. ? 1. : thick * 128. / (dist - rad);
	return d;
}

bool TexSphere::sunAtmosphere(const Viewer &vw)const{
	return const_cast<TexSphere*>(this)->calcDist(vw) - rad < atmodensity * 10.;
}
