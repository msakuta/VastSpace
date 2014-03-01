/** \file
 * Definition of TorusStation class and its companions.
 */
#ifndef TORUSSTATION_H
#define TORUSSTATION_H
#include "astro.h"
#include "Entity.h"
#include "Docker.h"
#include "ModelEntity.h"
#include <btBulletDynamicsCommon.h>


class TorusStationEntity;
struct Model;


/// \brief A space colony similar to Stanford Torus or the station in 2001 space odyssey.
///
/// This class derives Astrobj but is treated as an Entity, too. The Entity aspect is defined as separate class TorusStationEntity,
/// not as multiple inheritance.
class VASTSPACE_EXPORT TorusStation : public Astrobj{
public:
//	static const unsigned classid;
	static ClassRegister<TorusStation> classRegister;
	typedef Astrobj st;
	friend class TorusStationEntity;

	double sun_phase;
	TorusStationEntity *ent;

	// Constructors
	TorusStation(Game *game);
	TorusStation(const char *path, CoordSys *root);

	// Overridden methods
	virtual ~TorusStation();
//	virtual const char *classname()const{return "TorusStation";}
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

	static double RAD;
	static const double THICK;
	static const double stackInterval;
	static const int segmentCount;
	static const int stackCount = 2; ///< Count of torus wheels stacked along the rotation axis.
	static const bool alternatingDirection = false; ///< Alternate rotation directions of the stacked wheels.

	static double getZOffsetStack(int n){
		return (n - (stackCount - 1) * 0.5) * stackInterval;
	}

	bool cull(const Viewer &vw)const;

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
	Mat4d transform(const Viewer *vw)const;
	bool cullQuad(const Vec3d (&pos)[4], const GLcull *gc2, const Mat4d &mat);
	GLuint compileWallTexture();
	void beginWallTexture(const Viewer *);
	void endWallTexture();

	static Model *model;
	static double defaultMass;
};


class TorusStationDocker;

/// \brief TorusStation companion Entity.
///
/// If this object exists, corresponding TorusStation class is always present,
/// but the revese is not necessarily true.
/// It's not registered as a user-creatable object, but will be automatically created when TorusStation class is created.
class TorusStationEntity : public Entity{
public:
	typedef Entity st;
	friend class TorusStation;
	static EntityRegister<TorusStationEntity> entityRegister;
	static const double hubRadius; ///< Really should be derived from hub model
	static const double segmentOffset; ///< Offset of model from TorusStation::RAD
	static const double segmentBaseHeight; ///< Really should be derived from segment model
	static const int portHitPartOffset = 100;
	TorusStation *astro;
	TorusStationEntity(Game *game);
	TorusStationEntity(WarField *w, TorusStation &astro);
	void init();
	EntityStatic &getStatic()const{return entityRegister;}
	virtual ~TorusStationEntity();
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual double getHitRadius()const{return hitRadius;}
	virtual void enterField(WarField *);
	virtual bool isTargettable()const{return true;}
	virtual bool isSelectable()const{return true;}
	virtual double getMaxHealth()const{return maxHealthValue;}
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int takedamage(double damage, int hitpart);
	virtual void draw(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual Props props()const;
	virtual int popupMenu(PopupMenu &list);
	virtual bool command(EntityCommand *com);

	struct HitCylinder{
		Vec3d org; ///< Origin position of the cylinder.
		Vec3d axis; ///< Axis and half-length of the cylinder.
		double radius;
		HitCylinder(Vec3d aorg = Vec3d(0,0,0), Vec3d aaxis = Quatd(0,0,0,1), double radius = 0) : org(aorg), axis(aaxis), radius(radius){}
	};

protected:
	btCompoundShape *btshape;
	HitBoxList hitboxes;
	std::vector<HitCylinder> hitcylinders;
	TorusStationDocker *docker;
	static suf_t *sufdock;
	static double modelScale;
	static double hitRadius;
	static double maxHealthValue;
	static GLuint overlayDisp;
	virtual Docker *getDockerInt();
	void buildShape();
	struct ModelSet;
	static const ModelSet &loadModels();
};

/// \brief The docking port object  for TorusStation.
class TorusStationDocker : public Docker{
public:
	typedef Docker st;
	static const unsigned classid;
	TorusStationDocker(Game *game) : st(game){}
	TorusStationDocker(TorusStationEntity * = NULL);
	virtual const char *classname()const;
	virtual void dock(Dockable *d);
	virtual Vec3d getPortPos(Dockable *)const;
	virtual Quatd getPortRot(Dockable *)const;
};

/// Instantiate the constructor for TorusStationEntity to prevent creating Entity
/// without corresponding TorusStation.
template<> Entity *Entity::EntityRegister<TorusStationEntity>::create(WarField *){
	return NULL;
}


#endif
