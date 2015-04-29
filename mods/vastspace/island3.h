/** \file
 * \brief Definition of Island3 and its companion classes.
 */
#ifndef ISLAND3_H
#define ISLAND3_H
#include "astro.h"
#include "Entity.h"
#include "Docker.h"
#include "Model-forward.h"
#include <btBulletDynamicsCommon.h>
extern "C"{
#include <clib/suf/suf.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufvbo.h>
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
class VASTSPACE_EXPORT Island3 : public Astrobj{
public:
//	static const unsigned classid;
	static ClassRegister<Island3> classRegister;
	typedef Astrobj st;
	friend class Island3Entity;

	double sun_phase;
	Island3Entity *ent;
	Island3Building *bldgs[ISLAND3_BUILDINGS];
//	btRigidBody *bbody;

	// Constructors
	Island3(Game *game);
	Island3(const char *path, CoordSys *root);

	// Overridden methods
	virtual ~Island3();
//	virtual const char *classname()const{return "Island3";}
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual bool belongs(const Vec3d &pos)const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void predraw(const Viewer *);
	virtual void draw(const Viewer *);
	virtual void drawtra(const Viewer *);
	virtual void onChangeParent(CoordSys *oldParent);
	virtual bool readFile(StellarContext &, int argc, const char *argv[]);
	virtual const Static &getStatic()const{return classRegister;}

	// Public methods
//	CoverPointVector getCoverPoint(const Vec3d &org, double radius);

	static int &g_shader_enable;
protected:
	double rotation; ///< Rotation of the cylinder
	int race; ///< Equivalent to ent->race
	int gases; ///< Resource units
	int solids; ///< Resource units
	int people; ///< Population of this colony
	bool headToSun;
	bool cullLevel; ///< -1 = indeterminant, 0 = near, 1 = far

	void init();
	int getCutnum(const Viewer *vw)const;
	void calcWingTrans(int i, Quatd &rot, Vec3d &pos);
	Mat4d transform(const Viewer *vw)const;
	bool cullQuad(const Vec3d (&pos)[4], const GLcull *gc2, const Mat4d &mat);
	GLuint compileWallTexture();
	void beginWallTexture(const Viewer *);
	void endWallTexture();
	static GLuint walllist;
	static GLuint walltex;
	static Model *bridgetower;
};


class Island3Docker;

/// \brief Island3 companion Entity.
///
/// If this object exists, corresponding Island3 class is always present,
/// but the revese is not necessarily true.
/// It's not registered as a user-creatable object, but will be automatically created when Island3 class is created.
class Island3Entity : public Entity{
public:
	typedef Entity st;
	friend Island3;
	static EntityRegisterNC<Island3Entity> entityRegister;
	Island3 *astro;
	Island3Entity(Game *game);
	Island3Entity(WarField *w, Island3 &astro);
	virtual ~Island3Entity();
	virtual const char *classname()const{return "Island3Entity";} ///< Overridden because getStatic() is not defined
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual double getHitRadius()const{return 20000.;}
	virtual void enterField(WarField *);
	virtual bool isTargettable()const{return true;}
	virtual bool isSelectable()const{return true;}
	virtual double getMaxHealth()const{return 1e6;}
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int takedamage(double damage, int hitpart);
	virtual void draw(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual Props props()const;
	virtual int popupMenu(PopupMenu &list);
	virtual bool command(EntityCommand *);
protected:
	btCompoundShape *btshape;
	btBoxShape *wings[3];
	btTransform wingtrans[3];
	Island3Docker *docker;
	static suf_t *sufdock;
	virtual Docker *getDockerInt();
	void buildShape();
	static suf_t *loadModel(suf_t *(*sufs)[3], VBO *(*vbo)[3], suftex_t *(*pst)[3]);
};

class Island3Docker : public Docker{
public:
	typedef Docker st;
	static const unsigned classid;
	Island3Docker(Game *game) : st(game){}
	Island3Docker(Island3Entity * = NULL);
	virtual const char *classname()const;
	virtual void dock(Dockable *d);
	virtual Vec3d getPortPos(Dockable *)const;
	virtual Quatd getPortRot(Dockable *)const;
};


#endif
