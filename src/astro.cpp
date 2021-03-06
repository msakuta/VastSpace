/** \file
 * \brief Implementation of astronomical objects and their movements.
 *
 * Drawing methods are separated to another source, astrodraw.cpp.
 *
 * \sa astrodraw.cpp
 */
#define NOMINMAX
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
#include "CoordSys-property.h"
#include "CoordSys-sq.h"
#include "Game.h"
#include "sqserial.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/cfloat.h>
}
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sstream>
#include <fstream>
#include <algorithm>


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
	static bool sq_define(HSQUIRRELVM);
	ClassId classname()const{return classRegister.id;}
	void updateInt(double dt);
	Barycenter *toBarycenter(){return this;}
};

const ClassRegister<Barycenter> Barycenter::classRegister("Barycenter", sq_define);


bool Barycenter::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	if(SQ_SUCCEEDED(sq_get(v, 1))){
		sq_poptop(v);
		return false;
	}
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("constructor"), sq_CoordSysConstruct<Barycenter>);
	sq_createslot(v, -3);
	return true;
}

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

const ClassRegister<OrbitCS> OrbitCS::classRegister("Orbit", sq_define);

bool OrbitCS::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	if(SQ_SUCCEEDED(sq_get(v, 1))){
		sq_poptop(v);
		return false;
	}
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("constructor"), sq_CoordSysConstruct<OrbitCS>);
	register_closure(v, _SC("orbits"), [](HSQUIRRELVM v){
		OrbitCS *star = static_cast<OrbitCS*>(sq_refobj(v));
		SQFloat radius, eccentricity, phase;
		SQQuatd axis;
		if(SQ_FAILED(sq_getfloat(v, 3, &radius)))
			radius = 0;
		if(SQ_FAILED(sq_getfloat(v, 4, &eccentricity)))
			eccentricity = 0;
		axis.getValue(v, 5);
		if(SQ_FAILED(sq_getfloat(v, 6, &phase)))
			phase = 0;
		star->orbits(dynamic_cast<OrbitCS*>(sq_refobj(v, 2)), radius, eccentricity, axis.value, phase);
		return SQInteger(0);
	});
	sq_createslot(v, -3);
	return true;
}

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
	updateInt(dt);
	st::anim(dt);
}

void OrbitCS::clientUpdate(double dt){
	updateInt(dt);
	st::clientUpdate(dt);
}

/// \brief The common update procedure among server and client.
///
/// Derived classes can override this function to share the code among server and client.
/// The function entry point for updating could be shared from the beginning.
///
/// Remember that you cannot delete anything in the client, but other things go much like server.
void OrbitCS::updateInt(double dt){
	Universe *u = game->universe;
	double scale = u ? u->astro_timescale : 1./*timescale ? 5000. * pow(10., timescale-1) : 1.*/;
	OrbitCS *center = orbit_home ? (OrbitCS*)orbit_home : orbit_center ? orbit_center->toOrbitCS() : NULL;
	if(center && orbitType != NoOrbit){
		int timescale = 0;
		double dist;
		double omega;
		Vec3d orbpos, oldpos;
		Vec3d orbit_omg, omgdt;
		orbpos = parent->tocs(center->pos, center->parent);
		dist = orbit_rad;
		double systemMass = center->getSystemMass(this->toAstrobj());
		if(systemMass == 0)
			return;
		omega = scale / (dist * sqrt(dist / UGC / systemMass));
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
}

bool OrbitCS::readFileStart(StellarContext &){
	enable = 0;
	return true;
}

bool OrbitCS::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "orbits")){
		if(ps){
			OrbitCS *orbit = dynamic_cast<OrbitCS*>(parent->findcspath(ps));
			if(orbit)
				orbits(orbit);
			else
				CmdPrint(gltestp::dstring() << sc.fname << "(" << sc.line << "): OrbitCS::readFile: Warning: could not find parent orbit: " << ps);
		}
		return true;
	}
	else if(!strcmp(s, "orbit_radius") || !strcmp(s, "semimajor_axis")){
//		if(s = strtok(ps, " \t\r\n"))
			orbit_rad = sqcalc(sc, ps, s) * lengthUnit;
		return true;
	}
	else if(!strcmp(s, "eccentricity")){
		if(1 < argc){
			double d;
			d = sqcalc(sc, argv[1], s);
			eccentricity = d;
		}
		return true;
	}
	else if(!strcmp(s, "orbit_axis")){
		if(1 < argc)
			orbit_axis[0] = sqcalc(sc, argv[1], s);
		if(2 < argc)
			orbit_axis[1] = sqcalc(sc, argv[2], s);
		if(3 < argc){
			orbit_axis[2] = sqcalc(sc, argv[3], s);
			orbit_axis[3] = sqrt(std::max(0., 1. - VECSLEN(orbit_axis)));
		}
		return true;
	}
	else if(!strcmp(s, "orbit_inclination") || !strcmp(s, "inclination")){
		if(1 < argc){
			enable |= 1;
			inclination = sqcalc(sc, argv[1], s) / deg_per_rad;
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
			loan = sqcalc(sc, argv[1], s) / deg_per_rad;
		}
		return true;
	}
	else if(!strcmp(s, "argument_of_periapsis")){
		if(1 < argc){
			enable |= 4;
			aop = sqcalc(sc, argv[1], s) / deg_per_rad;
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

const CoordSys::PropertyMap &OrbitCS::propertyMap()const{
	static PropertyMap pmap = st::propertyMap();
	static bool init = false;
	if(!init){
		init = true;
		pmap[_SC("showOrbit")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const OrbitCS *a = static_cast<const OrbitCS*>(cs);
			sq_pushbool(v, a->getShowOrbit() ? SQTrue : SQFalse);
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			OrbitCS *a = static_cast<OrbitCS*>(cs);
			SQBool b;
			if(SQ_FAILED(sq_getbool(v, 3, &b)))
				return sq_throwerror(v, _SC("OrbitCS.showOrbit received non-boolean value"));
			a->setShowOrbit(!!b);
			return SQInteger(0);
		});

		// orbitCenter property is read-only because manipulating it carelessly can corrupt
		// entire structure.  Orbit calculation requires involving celestial bodies recognize each other.
		// So use orbits() method to specify orbital relations.
		pmap[_SC("orbitCenter")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			sq_pushobj(v, const_cast<CoordSys*>(static_cast<const OrbitCS*>(cs)->getOrbitCenter()));
			return SQInteger(1);
		}, NULL);

		// orbiters property is read-only because of the same reason as orbitCenter.
		pmap[_SC("orbiters")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const OrbitCS *orbit = static_cast<const OrbitCS*>(cs);
			sq_newarray(v, orbit->orbiters.size());
			int i = 0;
			for(auto it : orbit->orbiters){
				sq_pushinteger(v, i);
				sq_pushobj(v, orbit->orbiters[i]);
				if(SQ_FAILED(sq_set(v, -3)))
					return sq_throwerror(v, _SC("Error on constructing orbiters array"));
				i++;
			}
			return SQInteger(1);
		}, NULL);

		pmap[_SC("orbitRadius")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const OrbitCS *a = static_cast<const OrbitCS*>(cs);
			sq_pushfloat(v, SQFloat(a->orbit_rad));
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			OrbitCS *a = static_cast<OrbitCS*>(cs);
			SQFloat f;
			if(SQ_FAILED(sq_getfloat(v, 3, &f)))
				return sq_throwerror(v, _SC("OrbitCS.orbitRadius received non-float value"));
			a->orbit_rad = double(f);
			return SQInteger(0);
		});

		// Read-only property to return orbital period of this OrbitCS.
		pmap[_SC("orbitPeriod")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			do{
				const OrbitCS *a = static_cast<const OrbitCS*>(cs);
				const CoordSys *c = a->getOrbitCenter();
				while(!c && a){
					if(!a->parent)
						break;
					a = a->parent->toOrbitCS();
					if(!a)
						break;
					c = a->getOrbitCenter();
				}
				if(!c)
					break;
				const OrbitCS *oc = c->toOrbitCS();
				if(!oc)
					break;
				// Oddly enough, orbital period is dependent to semi major axis but not eccentricity.
				// http://en.wikipedia.org/wiki/Orbital_period
				sq_pushfloat(v, SQFloat(sqrt(4 * M_PI * M_PI / DEF_UGC / (oc->getSystemMass() + a->getSystemMass())
					* a->orbit_rad * a->orbit_rad * a->orbit_rad)));
				return SQInteger(1);
			}while(0);
			sq_pushfloat(v, 0);
			return SQInteger(1);
		}, NULL);
	}
	return pmap;
}

OrbitCS *OrbitCS::toOrbitCS(){
	return this;
}

Barycenter *OrbitCS::toBarycenter(){
	return NULL;
}

double OrbitCS::getSystemMass(const Astrobj *ignore)const{
	double ret = 0.;
	if(this != ignore){
		const Astrobj *a = toAstrobj();
		if(a)
			ret += a->mass;
	}
	for(auto it : orbiters)
		ret += it->getSystemMass(ignore);
	return ret;
}

void OrbitCS::orbits(OrbitCS *o, double radius, double eccentricity, const Quatd &axis, double phase){
	if(!o)
		return;
	Astrobj *a = o->toAstrobj();
	if(a)
		orbit_home = a;
	o->orbiters.push_back(this);
	if(o->toBarycenter()){
		orbitType = TwoBody;
		orbit_center = o;
	}
	else
		orbitType = Satellite;
	if(radius != 0.){
		orbit_rad = radius;
		this->eccentricity = eccentricity;
		orbit_axis = axis;
		orbit_phase = phase;
		updateInt(0); // Recalculate position with orbital elements
	}
}







void Barycenter::updateInt(double dt){
	if(orbiters.size() < 2){
		st::updateInt(dt);
		return;
	}

	// Pick the first 2 Astrobjs in orbiters since orbiters can contain non-Astrobj CoordSys which
	// has no mass and do not affect orbit calculation.
	Astrobj *a0 = NULL, *a1 = NULL;
	for(int i = 0; i < orbiters.size(); i++){
		if(!a0){
			if(orbiters[i]->toAstrobj())
				a0 = orbiters[i]->toAstrobj();
		}
		else if(!a1){
			if(orbiters[i]->toAstrobj())
				a1 = orbiters[i]->toAstrobj();
		}
		else if(orbiters[i]->toAstrobj()){
			// We cannot solve other than two-body problem.
			CmdPrint(gltestp::dstring() << _SC("Warning: I cannot solve three-body problem in ") << getpath());
			st::updateInt(dt);
			return;
		}
	}

	if(a0 && a1){
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
				// Two-body problem must synchronize orbit phase bettween involving bodies
				if(i)
					a->orbit_phase = a0->orbit_phase;
				else
					a->orbit_phase += omega * dt;

				Vec3d pos0(cos(a->orbit_phase), sin(a->orbit_phase), 0.);
				Quatd q = a->orbit_axis;
				a->pos = q.trans(pos0);
				// TODO: Radius should be deduced by mass ratio
				a->pos *= a->orbit_rad;
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
		st::updateInt(dt);
	}
}










Astrobj::Astrobj(const char *name, CoordSys *cs) : st(name, cs), mass(1e10), brightness(0.), basecolor(.5f,.5f,.5f,1.){
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
	sc.o << " " << brightness;
	sc.o << " " << basecolor;
}

void Astrobj::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> " " >> rad;
	sc.i >> " " >> mass;
	sc.i >> " " >> brightness;
	sc.i >> " " >> basecolor;
}

bool Astrobj::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "radius")){
		if(argv[1]){
			rad = sqcalc(sc, argv[1], s) * lengthUnit;
		}
		return true;
	}
	else if(!strcmp(s, "diameter")){
		if(argv[1])
			rad = .5 * sqcalc(sc, argv[1], s) * lengthUnit;
		return true;
	}
	else if(!strcmp(s, "mass")){
		if(argv[1]){
			mass = sqcalc(sc, argv[1], s);
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
		COLOR32 c = strtoul(ps, NULL, 16);
		basecolor = Vec4f(COLOR32B(c)/256.f,COLOR32G(c)/256.f,COLOR32R(c)/256.f,1.);
	}
	else if(!strcmp(s, "basecolor")){
		if(3 < argc){
			basecolor = Vec4f(
				float(sqcalc(sc, argv[1], s)),
				float(sqcalc(sc, argv[2], s)),
				float(sqcalc(sc, argv[3], s)),
				1.f);
		}
		return true;
	}
	else if(!strcmp(s, "absolute_magnitude") || !strcmp(s, "absmag")){
		setAbsMag(sqcalc(sc, ps, s));
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

const double Astrobj::sol_absmag = 4.83;

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


static int tocs_children_invokes = 0;

double checkEclipse(const Astrobj *a, const CoordSys *retcs, const Vec3d &src, const Vec3d &lightSource, const Vec3d &ray, const Astrobj **rethit){
	const CoordSys *eis = const_cast<CoordSys*>(retcs)->findeisystem();
	if(!eis)
		return 1.;
	for(CoordSys::AOList::const_iterator it = eis->aorder.begin(); it != eis->aorder.end(); ++it){
		const Astrobj *ahit = (*it)->toAstrobj();
		if(ahit && ahit != a){

			// It's lacking evidence why this value is appropriate, but we do not want really small objects (compared to celestial objects!) to
			// cast a shadow to cause eclipses. Shadows in that scale are handled by shadow mapping.
			if(ahit->rad < 10.)
				continue;

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
			double sp = FLT_EPSILON < obsVec.len() ? rayOffset / obsVec.len() /*rayDir.sp(obsVec.norm())*/ : 0;

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
				Vec3d delta = lightSource - ahitpos;
				if(delta.slen() != 0){
					double penumbraRadius = a->rad * rayOffset / delta.len();
					double penumbraInner = ahit->rad - penumbraRadius;
					double penumbraOuter = ahit->rad + penumbraRadius;
					double penumbraRange = penumbraOuter - penumbraInner;
					penumbra = hitdist <= penumbraOuter ? penumbraRange == 0. ? 0. : (hitdist - penumbraInner) / penumbraRange : 1.;
				}
			}

			if(rethit)
				*rethit = ahit;

			// Scattering by atmosphere can attenuate the direct light down to 25%. (Is it right?)
			return rangein(penumbra - 0.75 * scatter, 0, 1);
		}
	}
	return 1.;
}

bool FindBrightestAstrobj::invoke(const CoordSys *cs2){
	const Astrobj *a = cs2->toAstrobj()/*dynamic_cast<Astrobj*>(cs2)*/;
	if(!a || a == ignorecs ||  30 <= a->getAbsMag())
		return true;

	// I don't understand why, but this function seems to be occasionally called twice for the same
	// argument in a single find().
	for(int i = 0; i < results.size(); i++){
		if(results[i].cs == a)
			return true;
	}

	Vec3d lightSource = retcs->tocs(vec3_000, a);
	Vec3d ray = src - lightSource;
	double sd = ray.slen();
	if(sd == 0.)
		return true;

	// Obtain brightness in ratio to Sun looked from earth
	double rawval = a->brightness * AU_PER_METER * AU_PER_METER / sd;

	// Omit further investigation if the raw brightness is below the threshold.
	if(rawval < threshold)
		return true;

	// Check for eclipses only if you could be the brightest celestial object.
	double val;
	if(checkEclipse && eclipseThreshold < rawval && brightness < rawval)
		val = rawval * ::checkEclipse(a, retcs, src, lightSource, ray, &eclipseCaster);
	else
		val = rawval;
	if(val == 0.)
		return true;

	if(results.size() < resultCount || results.back().brightness < val){
		nonShadowBrightness = rawval;
		ResultSet res;
		res.cs = a;
		res.brightness = val;
		res.pos = lightSource;
		std::vector<ResultSet>::iterator it = std::upper_bound(results.begin(), results.end(), val, [](const double &v, const ResultSet &it){
			return it.brightness < v;
		});
		results.insert(it, res);

		// Update most bright brightness
		brightness = results.front().brightness;

		if(resultCount < results.size())
			results.pop_back();
	}
	return true;
}

/// \brief Find the nearest brightest celestial object to this CoordSys.
///
/// The celestial object is usually a Star, but can be other things that reflects Star's light.
///
/// This function lacks ability to specify various options to find the desired Astrobjs.
/// Consider using FindBrightestAstrobj and CoordSys::find() instead of this function.
///
/// \param pos The position to measure the brightnesses at.
Astrobj *CoordSys::findBrightest(const Vec3d &po){
	FindBrightestAstrobj fba(this, pos);
	find(fba);
	if(fba.results.size())
		return const_cast<Astrobj*>(fba.results[0].cs);
	else
		return NULL;
}

bool Astrobj::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	if(SQ_SUCCEEDED(sq_get(v, 1))){
		sq_poptop(v);
		return false;
	}
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("constructor"), sq_CoordSysConstruct<Astrobj>);
	sq_createslot(v, -3);
	return true;
}

const CoordSys::PropertyMap &Astrobj::propertyMap()const{
	static PropertyMap pmap = st::propertyMap();
	static bool init = false;
	if(!init){
		init = true;
		pmap["radius"] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const Astrobj *a = static_cast<const Astrobj*>(cs);
			sq_pushfloat(v, a->rad);
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			Astrobj *a = static_cast<Astrobj*>(cs);
			SQFloat f;
			if(SQ_FAILED(sq_getfloat(v, 3, &f)))
				return sq_throwerror(v, _SC("Astrobj.radius could not convert to float"));
			a->rad = f;
			return SQInteger(0);
		});
		pmap["mass"] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			// Not dynamic_cast because it should be the correct type.
			const Astrobj *a = static_cast<const Astrobj*>(cs);
			sq_pushfloat(v, a->mass);
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			// Not dynamic_cast because it should be the correct type.
			Astrobj *a = static_cast<Astrobj*>(cs);
			SQFloat f;
			if(SQ_FAILED(sq_getfloat(v, 3, &f)))
				return sq_throwerror(v, _SC("Astrobj.rad set fail"));
			a->mass = f;
			return SQInteger(0);
		});
		pmap["absmag"] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const Astrobj *a = static_cast<const Astrobj*>(cs);
			sq_pushfloat(v, a->getAbsMag());
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			Astrobj *a = static_cast<Astrobj*>(cs);
			SQFloat f;
			if(SQ_FAILED(sq_getfloat(v, 3, &f)))
				return sq_throwerror(v, _SC("Astrobj.absmag set fail"));
			a->setAbsMag(f);
			return SQInteger(0);
		});
	}
	return pmap;
}


Star::Star(const char *name, CoordSys *cs) : Astrobj(name, cs), spect(Unknown), subspect(0){ brightness = 1.; }

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

bool Star::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	if(SQ_SUCCEEDED(sq_get(v, 1))){
		sq_poptop(v);
		return false;
	}
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("constructor"), sq_CoordSysConstruct<Star>);

	// Expose conversion functions between spectral type index and name.
	// Note that the functions are static; you don't need a Star instance to call them.
	register_closure(v, _SC("nameToSpectral"), [](HSQUIRRELVM v){
		const SQChar *str;
		if(SQ_FAILED(sq_getstring(v, 2, &str)))
			return sq_throwerror(v, _SC("nameToSpectral parameter is not string"));
		sq_pushinteger(v, nameToSpectral(str));
		return SQInteger(1);
	});
	register_closure(v, _SC("spectralToName"), [](HSQUIRRELVM v){
		SQInteger i;
		if(SQ_FAILED(sq_getinteger(v, 2, &i)))
			return sq_throwerror(v, _SC("spectralToName parameter is not int"));
		sq_pushstring(v, spectralToName(SpectralType(i)), -1);
		return SQInteger(1);
	});
	register_static(v, _SC("maxSpectralTypes"), Num_SpectralType);
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

const CoordSys::PropertyMap &Star::propertyMap()const{
	static PropertyMap pmap = st::propertyMap();
	static bool init = false;
	if(!init){
		init = true;
		pmap["spect"] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const Star *a = static_cast<const Star*>(cs);
			sq_pushstring(v, spectralToName(a->spect), -1);
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			Star *a = static_cast<Star*>(cs);
			const SQChar *str;
			if(SQ_FAILED(sq_getstring(v, 3, &str)))
				return sq_throwerror(v, _SC("Star.spect set fail"));
			a->spect = nameToSpectral(str);
			return SQInteger(0);
		});

		// We want some way to specify the spectral type by an integral value for use by scripts.
		pmap["spectIndex"] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){
			const Star *a = static_cast<const Star*>(cs);
			sq_pushinteger(v, SQInteger(a->spect));
			return SQInteger(1);
		},
		[](HSQUIRRELVM v, CoordSys *cs){
			Star *a = static_cast<Star*>(cs);
			SQInteger i;
			if(SQ_FAILED(sq_getinteger(v, 3, &i)))
				return sq_throwerror(v, _SC("Star.spectIndex set fail"));
			a->spect = SpectralType(i);
			return SQInteger(0);
		});
	}
	return pmap;
}

double Star::appmag(const Vec3d &pos, const CoordSys &cs)const{
	double dist = tocs(pos, &cs).len();
	double parsecs = dist / 30.857e12;
	return getAbsMag() + 5 * (log10(parsecs) - 1);
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

