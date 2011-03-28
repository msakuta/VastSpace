#ifndef ISLAND3_H
#define ISLAND3_H
#include "astro.h"
#include "Entity.h"
#include <btBulletDynamicsCommon.h>
extern "C"{
#include <clib/suf/suf.h>
}

#ifndef NDEBUG
#define ISLAND3_BUILDINGS 32
#else
#define ISLAND3_BUILDINGS 128
#endif


class Island3Entity;
class Island3Building;



/// \brief Space colony Island3, A.K.A. O'Neill Cylinder.
///
/// This class derives Astrobj but is treated as an Entity, too. The Entity aspect is defined as separate class Island3Entity,
/// not as multiple inheritance.
class Island3 : public Astrobj{
public:
//	static const unsigned classid;
	static ClassRegister<Island3> classRegister;
	typedef Astrobj st;
	friend class Island3Entity;

	double sun_phase;
	Island3Entity *ent;
	Island3Building *bldgs[ISLAND3_BUILDINGS];
//	btRigidBody *bbody;
	btCompoundShape *btshape;
	btBoxShape *wings[3];
	btTransform wingtrans[3];

	Island3();
	Island3(const char *path, CoordSys *root);
	virtual ~Island3();
	virtual const char *classname()const{return "Island3";}
//	virtual void init(const char *path, CoordSys *root);
	virtual bool belongs(const Vec3d &pos)const;
	virtual void anim(double dt);
	virtual void predraw(const Viewer *);
	virtual void draw(const Viewer *);
	virtual void drawtra(const Viewer *);
	virtual void onChangeParent(CoordSys *oldParent);
	virtual bool readFile(StellarContext &, int argc, const char *argv[]);

	static int &g_shader_enable;
protected:
	int race; ///< Equivalent to ent->race
	bool headToSun;
	bool cullLevel; ///< -1 = indeterminant, 0 = near, 1 = far

	int getCutnum(const Viewer *vw)const;
	void calcWingTrans(int i, Quatd &rot, Vec3d &pos);
	Mat4d transform(const Viewer *vw)const;
	bool cullQuad(const Vec3d (&pos)[4], const GLcull *gc2, const Mat4d &mat);
	void beginWallTexture(const Viewer *);
	void endWallTexture();
	static GLuint walllist, walltex;
	static suf_t *sufbridgetower;
};

/// \brief Island3 companion Entity.
///
/// If this object exists, corresponding Island3 class is always present,
/// but the revese is not necessarily true.
/// It's not registered as a user-creatable object, but will be automatically created when Island3 class is created.
class Island3Entity : public Entity{
public:
	typedef Entity st;
	friend Island3;
	Island3 &astro;
	Island3Entity(Island3 &astro);
	virtual ~Island3Entity();
	virtual const char *classname()const{return "Island3Entity";} ///< Overridden because getStatic() is not defined
	virtual double hitradius()const{return 20.;}
	virtual bool isTargettable()const{return true;}
	virtual bool isSelectable()const{return true;}
	virtual double maxhealth()const{return 1e6;}
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int takedamage(double damage, int hitpart);
	virtual void draw(wardraw_t *);
};


#endif
