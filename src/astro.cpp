/** \file
 * \brief Implementation of astronomical objects and their movements.
 *
 * Drawing methods are separated to another source, astrodraw.cpp.
 *
 * \sa astrodraw.cpp
 */
#include "astro.h"
#include "serial_util.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "astrodef.h"
#include "Player.h"
#include "cmd.h"
#include "sqadapt.h"
#include "Universe.h"
#include "judge.h"
#include "CoordSys-find.h"
#include "Game.h"
extern "C"{
#include "calc/calc.h"
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sstream>
#include <fstream>


/// \brief Barycenter of multi-body problems.
///
/// It was a shame to define the barycenter as a class, because it's not real object but imaginary frame of reference.
/// But it's not good approach to have each of the involving objects calculate physics each other, because there're
/// problem of calculation order. Therefore it ended up making all necessary computation collected into a new
/// class here.
class Barycenter : public OrbitCS{
public:
	typedef OrbitCS st;
	Barycenter(Game *game) : st(game){}
	Barycenter(const char *path, CoordSys *root) : st(path, root){}
	static const ClassRegister<Barycenter> classRegister;
	ClassId classname()const{return classRegister.id;}
	void anim(double dt);
	Barycenter *toBarycenter(){return this;}
};

const ClassRegister<Barycenter> Barycenter::classRegister("Barycenter");



OrbitCS::OrbitCS(const char *path, CoordSys *root) : st(path, root), orbit_center(NULL), orbitType(NoOrbit){
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
	sc.o << orbit_rad << orbit_home << orbit_center << orbit_axis;
	sc.o << orbit_phase;
	sc.o << eccentricity; /* orbital element */
	sc.o << flags2;
}

void OrbitCS::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> orbit_rad >> orbit_home >> orbit_center >> orbit_axis;
	sc.i >> orbit_phase;
	sc.i >> eccentricity; /* orbital element */
	sc.i >> flags2;
}

void OrbitCS::anim(double dt){
	Universe *u = game->universe;
	double scale = u ? u->astro_timescale : 1./*timescale ? 5000. * pow(10., timescale-1) : 1.*/;
	if(orbit_home && orbitType != NoOrbit){
		int timescale = 0;
		double dist;
		double omega;
		Vec3d orbpos, oldpos;
		Vec3d orbit_omg, omgdt;
		orbpos = parent->tocs(orbit_home->pos, orbit_home->parent);
		dist = orbit_rad;
		omega = scale / (dist * sqrt(dist / UGC / orbit_home->mass));
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
	st::anim(dt);
}

bool OrbitCS::readFileStart(StellarContext &){
	enable = 0;
	return true;
}

bool OrbitCS::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "orbits")){
		if(argv[1]){
			CoordSys *cs = parent->findcspath(ps);
			if(cs){
				orbit_home = cs->toAstrobj();
				OrbitCS *o = cs->toOrbitCS();
				if(o){
					o->orbiters.push_back(this);
					if(o->toBarycenter())
						orbit_center = o;
					else
						orbitType = Satellite;
				}
			}
		}
		return true;
	}
	else if(!strcmp(s, "orbit_radius") || !strcmp(s, "semimajor_axis")){
//		if(s = strtok(ps, " \t\r\n"))
			orbit_rad = calc3(&ps, sc.vl, NULL);
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
//	if(inclination == 0.)
//		orbit_axis = quat_u;
	else{
		Quatd qinc(sin(inclination / 2.), 0., 0., cos(inclination / 2.));
		Quatd qloan = enable & 2 ? Quatd(0., 0., sin(loan / 2.), cos(loan / 2.)) : quat_u;
		Quatd qaop = enable & 4 ? Quatd(0., 0., sin(aop / 2.), cos(aop / 2.)) : quat_u;
		orbit_axis = qaop * qinc * qloan;
	}
	return true;
}

OrbitCS *OrbitCS::toOrbitCS(){
	return this;
}

Barycenter *OrbitCS::toBarycenter(){
	return NULL;
}








void Barycenter::anim(double dt){
	// We cannot solve other than two-body problem.
	if(orbiters.size() == 2 && orbiters[0]->toAstrobj() && orbiters[1]->toAstrobj()){
		Astrobj *a0 = orbiters[0]->toAstrobj(), *a1 = orbiters[1]->toAstrobj();
		double rmass = a0->mass * a1->mass / (a0->mass + a1->mass);
		double scale = game->universe ? game->universe->astro_timescale : 1.;
		int timescale = 0;
		Vec3d omgdt;
		for(int i = 0; i < 2; i++){
			Astrobj *a = i ? a1 : a0;
			Vec3d orbpos = Vec3d(0,0,0);
			double dist = a->orbit_rad;
			double omega = scale / (dist * sqrt(dist / UGC / rmass));
			Vec3d orbit_omg = a->orbit_axis.norm();
			Vec3d oldpos = a->pos;
			if(a->eccentricity == 0.){
				a->orbit_phase += omega * dt;
				Quatd rot(0., 0., sin(a->orbit_phase / 2.), cos(a->orbit_phase / 2.));
				Quatd q = orbit_axis * rot;
				a->pos = q.trans(vec3_010);
				a->pos *= orbit_rad;
			}
			else{
				Vec3d pos0(cos(a->orbit_phase), sin(a->orbit_phase), 0.);
				Mat4d smat = mat4_u;
				double smia = a->orbit_rad * sqrt(1. - a->eccentricity * a->eccentricity); // semi-major axis
				smat.scalein(a->orbit_rad, smia, a->orbit_rad);
				smat.translatein(a->eccentricity, 0, 0);
				Mat4d rmat = a->orbit_axis.tomat4();
				Mat4d mat = rmat * smat;
				a->pos = mat.vp3(pos0);
//				a->pos *= rmass / a->mass;
				double r = a->pos.len() * a->mass / rmass;

				// Two-body problem must synchronize orbit phase bettween involving bodies
				if(i)
					a->orbit_phase = a0->orbit_phase;
				else if(DBL_MIN < r)
					a->orbit_phase += omega * dt * dist / r;
			}
			a->pos += orbpos;
			if(dt){
				a->velo = a->pos - oldpos;
				a->velo *= 1. / dt;
			}
			// Revolving, do such a thing yourself
//			a->rot = a->rot.quatrotquat(a->omg * dt * astro_timescale);
		}
		st::anim(dt);
	}
}










Astrobj::Astrobj(const char *name, CoordSys *cs) : st(name, cs), mass(1e10), absmag(30), basecolor(.5f,.5f,.5f,1.){
}

const ClassRegister<Astrobj> Astrobj::classRegister("Astrobj", sq_define);

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

bool Astrobj::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
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
/*		if(s = strtok(ps, " \t\r\n"))*/{
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
	else if(!strcmp(s, "absolute_magnitude") || !strcmp(s, "absmag")){
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


CoordSys::FindParam CoordSys::defaultFindParam;

static int tocs_children_invokes = 0;

struct Param{
	Astrobj *ret;
	double brightness;
	const CoordSys::FindParam &param; ///< Input parameters are internally read-only.
	
	Param(CoordSys::FindParam &param) : ret(NULL), brightness(0.), param(param){}
};

double checkEclipse(Astrobj *a, const CoordSys *retcs, const Vec3d &src, const Vec3d &lightSource, const Vec3d &ray){
	const CoordSys *eis = const_cast<CoordSys*>(retcs)->findeisystem();
	for(CoordSys::AOList::const_iterator it = eis->aorder.begin(); it != eis->aorder.end(); ++it){
		const Astrobj *ahit = (*it)->toAstrobj();
		if(ahit && ahit != a){
			Vec3d ahitpos = retcs->tocs(ahit->pos, ahit->parent);
			double hitdist;
			bool hit = jHitSpherePos(ahitpos, ahit->rad, src, -ray, 1., NULL, NULL, &hitdist);

			// Direction of the ray (unit vector).
			Vec3d rayDir = ray.norm();

			// Obstacle's vector
			Vec3d obsVec = src - ahitpos;

			// Distance of the camera and the light source, with offset 0 at the same distance of
			// the obstacle and the light source.
			double rayOffset = rayDir.sp(obsVec);

			// This scalar product indicates how the camera oriented against obstacle and light source.
			// -1 if you're directly behind, 1 if you're directly in between the obstacle and the sun,
			// and 0 if the three points form a right angle.
			// Obtained by dividing rayOffset by obsVec length for performance, but is really a scalar product.
			double sp = rayOffset / obsVec.len() /*rayDir.sp(obsVec.norm())*/;

			Viewer vw;
			vw.cs = retcs;
			vw.pos = src;
			// Obtain atmospheric scattering of sun light.
			double scatter = ahit->atmoScatter(vw);

			// The sun's light will be scattered most near horizon.
			scatter *= 1. - sp * sp;

			// Consider penumbra expansion as the shadow caster approaches the light source.
			double penumbra;
			if(rayOffset < 0.)
				penumbra = 1.;
			else{
				double penumbraRadius = a->rad * rayOffset / (lightSource - ahitpos).len();
				double penumbraInner = ahit->rad - penumbraRadius;
				double penumbraOuter = ahit->rad + penumbraRadius;
				penumbra = hitdist <= penumbraOuter ? (hitdist - penumbraInner) / (penumbraOuter - penumbraInner) : 1.;
			}

			// Scattering by atmosphere can attenuate the direct light down to 25%. (Is it right?)
			return rangein(penumbra - 0.75 * scatter, 0, 1);
		}
	}
	return 1.;
}

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
			Vec3d lightSource = retcs->tocs(vec3_000, a);
			Vec3d ray = src - lightSource;
			double sd = ray.slen();
			val = 0. < sd ? pow(2.512, -1.*a->absmag) / sd : 0.;
			// Check for eclipses only if you could be the brightest celestial object.
			if(p.param.checkEclipse && p.param.threshold < val && p.brightness < val){
				val *= checkEclipse(a, retcs, src, lightSource, ray);
			}
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

/// \brief Find the nearest brightest celestial object to this CoordSys.
///
/// The celestial object is usually a Star, but can be other things that reflects Star's light.
///
/// \param pos The position to measure the brightnesses at.
/// \param param The parameters.
Astrobj *CoordSys::findBrightest(const Vec3d &pos, FindParam &param){
	Param p(param);
	findchildbr(p, this, pos, this, NULL);
	findparentbr(p, this, pos, this);

	// Return the brightness through the parameter object if told so.
	if(param.returnBrightness)
		param.brightness = p.brightness;

	return p.ret;
}

SQInteger Astrobj::sqf_get(HSQUIRRELVM v){
	Astrobj *p = static_cast<Astrobj*>(sq_refobj(v));
	if(!p)
		return SQ_ERROR;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	if(!strcmp(wcs, _SC("rad"))){
		sq_pushfloat(v, SQFloat(p->rad));
		return 1;
	}
	else
		return st::sqf_get(v);
}

bool Astrobj::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sizeof(WeakPtr<CoordSys>)); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("_get"), sqf_get);
	sq_createslot(v, -3);
	return true;
}


Star::Star(const char *name, CoordSys *cs) : Astrobj(name, cs), spect(Unknown), subspect(0){ absmag = 0; }

void Star::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << spect;
	sc.o << subspect;
}

void Star::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> (int&)spect;
	sc.i >> subspect;
}

const ClassRegister<Star> Star::classRegister("Star", sq_define);

SQInteger Star::sqf_get(HSQUIRRELVM v){
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	Star *p = static_cast<Star*>(sq_refobj(v));
	if(!p)
		return SQ_ERROR;
	if(!strcmp(wcs, _SC("spectral"))){
		sq_pushstring(v, cpplib::dstring() << spectralToName(p->spect) << p->subspect, -1);
		return 1;
	}
	else
		return st::sqf_get(v);
}

bool Star::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sizeof(WeakPtr<CoordSys>)); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("_get"), sqf_get);
	sq_createslot(v, -3);
	return true;
}

bool Star::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "spectral")){
		if(argv[1]){
			spect = nameToSpectral(ps, &subspect);
		}
		return true;
	}
	else
		return st::readFile(sc, argc, argv);
}

bool Star::readFileEnd(StellarContext &sc){
	return st::readFileEnd(sc);
}

double Star::appmag(const Vec3d &pos, const CoordSys &cs)const{
	double dist = tocs(pos, &cs).len();
	double parsecs = dist / 30.857e12;
	return absmag + 5 * (log10(parsecs) - 1);
}

Star::SpectralType Star::nameToSpectral(const char *name, float *subspect){
	SpectralType ret;
	switch(toupper(name[0])){
		case 'O': ret = O; break;
		case 'B': ret = B; break;
		case 'A': ret = A; break;
		case 'F': ret = F; break;
		case 'G': ret = G; break;
		case 'K': ret = K; break;
		case 'M': ret = M; break;
		default: ret = Unknown;
	}
	if(subspect)
		*subspect = atof(&name[1]);
	return ret;
}

const char *Star::spectralToName(SpectralType spect){
	switch(spect){
		case O: return "O";
		case B: return "B";
		case A: return "A";
		case F: return "F";
		case G: return "G";
		case K: return "K";
		case M: return "M";
	}
	return "Unknown";
}

Vec3f Star::spectralRGB(SpectralType spect){
	switch(spect){
		case O: return Vec3f(.5, .5, 1.);
		case B: return Vec3f(.75, .75, 1.);
		case A: return Vec3f(1., 1., 1.);
		case F: return Vec3f(1., 1., .75);
		case G: return Vec3f(1., 1., .5);
		case K: return Vec3f(1., .75, .4);
		case M: return Vec3f(1., .5, .3);
	}
	return Vec3f(1., 1., 1.);
}

#ifndef _WIN32
void Star::predraw(const Viewer *){
}

void Star::draw(const Viewer *){
}
#endif

