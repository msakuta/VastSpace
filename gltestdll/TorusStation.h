/** \file
 * Definition of TorusStation class and its companions.
 */
#ifndef TORUSSTATION_H
#define TORUSSTATION_H
#include "astro.h"
#include "Entity.h"
#include "Docker.h"
#include "Autonomous.h"
#include <btBulletDynamicsCommon.h>


class TorusStationEntity;
struct Model;


/// \brief A space colony similar to Stanford Torus or the station in 2001 space odyssey.
///
/// This class derives Astrobj but is treated as an Entity, too. The Entity aspect is defined as separate class TorusStationEntity,
/// not as multiple inheritance.
class GLTESTDLL_EXPORT TorusStation : public Astrobj{
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

	static const double RAD;
	static const double THICK;
	static const double stackInterval;
	static const int segmentCount;
	static const int stackCount = 2; ///< Count of torus wheels stacked along the rotation axis.
	static const bool alternatingDirection = false; ///< Alternate rotation directions of the stacked wheels.

	static double getZOffsetStack(int n){
		return (n - (stackCount - 1) * 0.5) * stackInterval;
	}
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
	static unsigned classid;
	static const double hubRadius; ///< Really should be derived from hub model
	static const double segmentOffset; ///< Offset of model from TorusStation::RAD
	static const double segmentBaseHeight; ///< Really should be derived from segment model
	TorusStation *astro;
	TorusStationEntity(Game *game);
	TorusStationEntity(WarField *w, TorusStation &astro);
	virtual ~TorusStationEntity();
	virtual const char *classname()const{return "TorusStationEntity";} ///< Overridden because getStatic() is not defined
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &));
	virtual double getHitRadius()const{return TorusStation::RAD;}
	virtual void enterField(WarField *);
	virtual bool isTargettable()const{return true;}
	virtual bool isSelectable()const{return true;}
	virtual double maxhealth()const{return 1e6;}
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int takedamage(double damage, int hitpart);
	virtual void draw(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual Props props()const;
	virtual int popupMenu(PopupMenu &list);

	/// \brief Retrieves root path for this extension module.
	static gltestp::dstring modPath(){return "gltestdll/";}

protected:
	btCompoundShape *btshape;
	HitBoxList hitboxes;
	TorusStationDocker *docker;
	static suf_t *sufdock;
	virtual Docker *getDockerInt();
	void buildShape();
	static Model *loadModel();
	static Model *loadHubModel();
	static Model *loadHubEndModel();
	static Model *loadJointModel();
	static Model *loadSpokeModel();
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


#endif
