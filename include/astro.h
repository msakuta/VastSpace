/** \file
 * \brief Definition of astronomical orbital CoordSys classes, OrbitCS and Astrobj.
 */
#ifndef ASTRO_H
#define ASTRO_H
#include "CoordSys.h"
#include <clib/aquat.h>
#include <clib/avec3.h>
#include <clib/colseq/color.h>

/* Codes relating astronautics. */

// astrobj::flags shares with coordsys::flags, so use bits only over 16
#define AO_DELETE         0x00000002 /* marked as to be deleted, share with coordsys */
#define AO_PLANET         0x00010000
#define AO_GRAVITY        0x00020000
#define AO_ALWAYSSHOWNAME 0x00040000
#define AO_SPHEREHIT      0x00080000
#define AO_BLACKHOLE      0x00100000 /* Blackhole */
#define AO_STARGLOW       0x00200000 /* Star that emits light */
#define AO_ELLIPTICAL     0x00800000 /* Orbits close to circle are treated so for speed. */
#define AO_ATMOSPHERE     0x01000000 /* Atmosphere is existing. */
#define AO_ASTROBJ        0x80000000 /* Distinguish coordsys with astrobj by this flag. */

typedef void (*astrobj_draw_proc)(struct astrobj *, const struct viewer *);

class Player;
struct StellarContext;
class Barycenter;

template<typename T, size_t ofs> class EmbeddedList;

template<typename T, size_t ofs> class EmbeddedListNode{
	T *next;
public:
	operator EmbeddedList<T, ofs>(){return EmbeddedList<T, ofs>(this);}
};
template<typename T, size_t ofs> class EmbeddedList{
	EmbeddedListNode<T, ofs> *const p;
public:
	EmbeddedList(EmbeddedListNode<T, ofs> *a) : p(a){}
	operator T&(){return *(T*)(((char*)p) - ofs);}
};

/// CoordSys of orbital motion
class EXPORT OrbitCS : public CoordSys{
public:
	double orbit_rad;
	Astrobj *orbit_home; ///< if orbitType == Satellite, it's the mother body. otherwise the other object involving in the two-body problem.
	CoordSys *orbit_center; ///< if orbitType == TwoBody, it's CoordSys of barycenter. otherwise not used.
	Quatd orbit_axis; ///< normal vector to orbit plane
	double orbit_phase; ///< phase at which position of a cycle
	double eccentricity; ///< orbital element
public:
	enum OrbitType{
		NoOrbit, ///< Not orbiting
		Satellite, ///< This object's mass is negligible compared to its mother body
		TwoBody, ///< The objects' mass are close enough to assume two-body problem
		Num_OrbitType
	};
	typedef CoordSys st;
	OrbitCS(Game *game) : st(game){}
	OrbitCS(const char *path, CoordSys *root);
	static const ClassRegister<OrbitCS> classRegister;
	virtual const Static &getStatic()const{return classRegister;}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual bool readFileStart(StellarContext &);
	virtual bool readFile(StellarContext &, int argc, const char *argv[]);
	virtual bool readFileEnd(StellarContext &);
	virtual OrbitCS *toOrbitCS();
	virtual Barycenter *toBarycenter();

	/// Returns if orbit center or home is available.
	CoordSys *getOrbitCenter(){return orbit_center ? (CoordSys*)orbit_center : (CoordSys*)orbit_home;}
	const CoordSys *getOrbitCenter()const{return orbit_center ? (CoordSys*)orbit_center : (CoordSys*)orbit_home;}

	bool getShowOrbit()const{return flags2 & OCS_SHOWORBIT;}
	void setShowOrbit(bool b);

protected:
//	EmbeddedListNode<OrbitCS, offsetof(OrbitCS, gravgroup)> gravgroup;
	virtual void updateInt(double dt);
	std::vector<OrbitCS*> orbiters;
	enum OrbitType orbitType;
	int enable;

	// OrbitCS::flags2
	static const int OCS_SHOWORBIT = 1;
	/// Hide internal flags from the public
	int flags2;

	double inclination, loan, aop;
};

/// Astronomical object. Usually orbits some other object.
class EXPORT Astrobj : public OrbitCS{
public:
	typedef OrbitCS st;

//	Quatd bodyrot; // Object's rotation (distinguished from rotation of underlying coordinate system)
//	Vec3d bodyomg; // Object's spin
	double rad;
	double mass;
	float absmag; /* Absolute Magnitude */
	Vec4f basecolor; /* rough approximation of apparent color */

	Astrobj(Game *game) : st(game){}
	Astrobj(const char *name, CoordSys *cs);
//	void init(const char *name, CoordSys *cs);

	virtual const Static &getStatic()const{return classRegister;}
	static bool sq_define(HSQUIRRELVM);
	static const ClassRegister<Astrobj> classRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual bool readFile(StellarContext &, int argc, const char *argv[]);
	virtual bool readFileEnd(StellarContext &);
	virtual Astrobj *toAstrobj(){ return this; }
	virtual double atmoScatter(const Viewer &vw)const{ return 0.; }
	virtual bool sunAtmosphere(const Viewer &vw)const{ return false; }

	/// \brief Returns approximate scale height of this astronomical object.
	///
	/// The scale height is the distance from the surface of an astronomical object that
	/// the atmospheric pressure decreases by a factor of e (the base of natural logarithms).
	/// http://en.wikipedia.org/wiki/Scale_height
	virtual double getAtmosphericScaleHeight()const{return 0.;}

	/// \brief Returns ambient luminosity based on the sun's distance.
	///
	/// The sun in this context is the brightest local Star.
	virtual double getAmbientBrightness(const Viewer &vw)const{ return 0.; }

protected:
	static SQInteger sqf_get(HSQUIRRELVM v);
};




//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

inline void OrbitCS::setShowOrbit(bool b){
	if(!b)
		flags2 &= ~OCS_SHOWORBIT;
	else
		flags2 |= OCS_SHOWORBIT;
}

#endif
