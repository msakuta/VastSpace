/** \file
 * \brief Definition of Lagrange Points.
 */
#include "serial_util.h"
#include "astro_star.h"

extern double gravityfactor;

/// Base class for Lagrange points
class LagrangeCS : public CoordSys{
public:
	typedef CoordSys st;
	LagrangeCS(Game *game) : st(game){}
	LagrangeCS(const char *path, CoordSys *root);
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool readFile(StellarContext &, int argc, const char *argv[]);
protected:
	Astrobj *objs[2];
};

/// L1 point of two celestial bodies
class Lagrange1CS : public LagrangeCS{
public:
	typedef LagrangeCS st;
	Lagrange1CS(Game *game) : st(game){}
	Lagrange1CS(const char *path, CoordSys *root) : st(path, root){}
	static const ClassRegister<Lagrange1CS> classRegister;
	virtual const Static &getStatic()const{return classRegister;}
	void anim(double dt);
};

/// L2 point of two celestial bodies
class Lagrange2CS : public LagrangeCS{
public:
	typedef LagrangeCS st;
	Lagrange2CS(Game *game) : st(game){}
	Lagrange2CS(const char *path, CoordSys *root) : st(path, root){}
	static const ClassRegister<Lagrange2CS> classRegister;
	virtual const Static &getStatic()const{return classRegister;}
	void anim(double dt);
};

/// Base class of Lagrange4CS and Lagrange5CS. Calculates offset along orbit plane.
/// Assumes object 2 orbits around orbit 1.
class LagrangeTrojan : public LagrangeCS{
public:
	typedef LagrangeCS st;
	LagrangeTrojan(Game *game) : st(game){}
	LagrangeTrojan(const char *path, CoordSys *root) : st(path, root){}
	void anim(double dt);
	virtual Vec3d offsetPos()const = 0;
};

/// Lagrange 4 point. Resides ahead of orbiting body.
class Lagrange4CS : public LagrangeTrojan{
public:
	typedef LagrangeTrojan st;
	Lagrange4CS(Game *game) : st(game){}
	Lagrange4CS(const char *path, CoordSys *root) : st(path, root){}
	static const ClassRegister<Lagrange4CS> classRegister;
	virtual Vec3d offsetPos()const{return Vec3d(0.5, sqrt(3.) / 2., 0.);}
};

/// Lagrange 5 point. Resides after the body.
class Lagrange5CS : public LagrangeTrojan{
public:
	typedef LagrangeTrojan st;
	Lagrange5CS(Game *game) : st(game){}
	Lagrange5CS(const char *path, CoordSys *root) : st(path, root){}
	static const ClassRegister<Lagrange5CS> classRegister;
	virtual Vec3d offsetPos()const{return Vec3d(0.5, -sqrt(3.) / 2., 0.);}
};


/* Lagrange 1 point between two bodies */
LagrangeCS::LagrangeCS(const char *path, CoordSys *root) : st(path, root){
	objs[0] = objs[1] = NULL;
}

void LagrangeCS::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << objs[0] << objs[1];
}

void LagrangeCS::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> objs[0] >> objs[1];
}

const ClassRegister<Lagrange1CS> Lagrange1CS::classRegister("Lagrange1");



void Lagrange1CS::anim(double dt){
	st::anim(dt);
	Vec3d delta, oldpos;

	if(!objs[0] || !objs[1])
		return;

	// If the system is being initialized, make sure objects' position is initialized too.
	// Danger of infinite recursion is avoided by the fact Lagrange points cannot be applied
	// to another.
	if(dt == 0.){
		objs[0]->anim(dt);
		objs[1]->anim(dt);
	}

	double alpha = objs[1]->mass / (objs[0]->mass + objs[1]->mass);
	double f = (1. - pow(alpha, 1. / 3.));

	Vec3d pos1 = parent->tocs(objs[0]->pos, objs[0]->parent);
	Vec3d pos2 = parent->tocs(objs[1]->pos, objs[1]->parent);

	oldpos = pos;
	delta = pos2 - pos1;
	pos = delta * f;
	pos += pos1;
	if(dt)
		velo = (pos - oldpos) * (1. / dt);
}

bool LagrangeCS::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "object1")){
		if(1 < argc){
			objs[0] = parent->findastrobj(argv[1]);
		}
	}
	else if(!strcmp(s, "object2")){
		if(1 < argc){
			objs[1] = parent->findastrobj(argv[1]);
		}
	}
	else
		return st::readFile(sc, argc, argv);
	return true;
}

const ClassRegister<Lagrange2CS> Lagrange2CS::classRegister("Lagrange2");

void Lagrange2CS::anim(double dt){
	st::anim(dt);
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
	if(dt)
		velo = (pos - oldpos) * (1. / dt);
}


const ClassRegister<Lagrange4CS> Lagrange4CS::classRegister("Lagrange4");
const ClassRegister<Lagrange5CS> Lagrange5CS::classRegister("Lagrange5");

void LagrangeTrojan::anim(double dt){
	st::anim(dt);
	static int init = 0;
	double f, alpha;

	if(!objs[0] || !objs[1])
		return;

	// We assume objs[1] orbits around objs[0].
	CoordSys *cs;
	for(cs = objs[1]; cs; cs = cs->parent) if(OrbitCS *o = cs->toOrbitCS()){
		if(o->orbit_home == objs[0])
			break;
	}
	if(!cs)
		return;

	Vec3d pos1 = parent->tocs(objs[0]->pos, objs[0]->parent);
	Vec3d pos2 = parent->tocs(objs[1]->pos, objs[1]->parent);
	Vec3d delta = pos2 - pos1;
	if(delta.slen() < DBL_EPSILON)
		return;

	Vec3d lpos = offsetPos();
	Vec3d xhat = delta;
	Vec3d zhat = parent->tocs(objs[1]->orbit_axis.trans(Vec3d(0,0,1)), objs[1]->parent, true).vp(xhat);
	Vec3d oldpos = pos;
	pos = lpos[0] * xhat + lpos[1] * zhat + pos1;
	if(dt)
		velo = (pos - oldpos) * (1. / dt);
}


