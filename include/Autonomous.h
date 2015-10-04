/** \file
 * \brief Definition of Autonomous class.
 */
#ifndef AUTONOMOUS_H
#define AUTONOMOUS_H

#include "ModelEntity.h"
#include "EntityRegister.h"
#include "Inventory-forward.h"
#ifndef DEDICATED
#ifdef _WIN32
#include <windows.h>
#endif
#include <gl/GL.h>
#else
typedef unsigned int GLuint;
#endif


/* List of spaceship tasks that all types of ship share. */
enum sship_task{
	sship_idle, /* Approach to stationary orbit; relative velocity zero against local coordsys. */
	sship_undock, /* Undocked and exiting */
	sship_undockque, /* Queue in a line of undock waiting. */
	sship_dock, /* Docking */
	sship_dockque, /* Queue in a line of dock waiting. */
	sship_moveto, /* Move to indicated location. */
	sship_parade, /* Parade formation. */
	sship_delta, // Delta formation
	sship_attack, /* Attack to some object. */
	sship_away, /* Fighters going away in attacking cycle. */
	sship_gather, /* Gather resources. */
	sship_harvest, /* Harvest resources. */
	sship_harvestreturn, /* Harvest resources. */
	sship_warp, // Warping
	num_sship_task
};

class Autonomous;

// Template instantiation to prevent EntityRegisterNC from trying to create a abstract class instance.
// The default EntityRegisterNC::stcreate() returns new object even if it's not creatable from Squirrel codes,
// because an Entity creation can be invoked from server messages.
// In this case, Autonomous is a abstract class, which means even the server never creates one,
// so we can safely ignore the case.
template<> Entity *Entity::EntityRegisterNC<Autonomous>::stcreate(Game *game){ return NULL; }

/// \brief An autonomous object, which means it can move around by itself, but not remote control like Missiles.
class EXPORT Autonomous : public ModelEntity{
public:
	typedef ModelEntity st;
	static EntityRegisterNC<Autonomous> entityRegister;
	Vec3d dest; // Move order destination
	enum sship_task task;
	int direction;

	Autonomous(Game *game) : st(game){}
	Autonomous(WarField *w);

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void addRigidBody(WarSpace*);
	virtual void anim(double dt);
	virtual void drawtra(wardraw_t *);
	virtual void drawHUD(wardraw_t *);
	virtual void control(const input_t *, double);
	unsigned analog_mask();
	virtual Props props()const;
	struct ManeuverParams;
	virtual const ManeuverParams &getManeuve()const;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual bool command(EntityCommand *com);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);

	virtual short getDefaultCollisionMask()const;

	/// \brief Returns velocity vector in absolute coordinates.
	///
	/// In the relative-space, there's no such thing like an absolute velocity, but it's used to measure
	/// inter-CoordSys movements, such as warping.
	virtual Vec3d absvelo()const;

	void maneuver(const Mat4d &mat, double dt, const ManeuverParams *mn);
	void steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale);
	const InventoryItemList &getInventory()const{return inventory;}
	InventoryItemList &getInventory(){return inventory;}

	/// \brief A set of fields that defines maneuverability of the ship.
	struct ManeuverParams{
		double accel; ///< Linear acceleration rate [m/s^2]
		double maxspeed; ///< Maximum linear speed [m/s], which wouldn't really exist in space, but provided for safe sailing.
		double angleaccel; ///< Angular acceleration [rad/s^2]
		double maxanglespeed; ///< Maximum angular speed [rad/s], which also wouldn't exist.
		double capacity; ///< The capacity of capacitor [MJ]
		double capacitor_gen; ///< Rate of generated energy [MW]

		/// An optional parameter to define anisotropic acceleration. Leaving them unspecified make them all zeros, which means
		/// falling back to default acceleration.
		double dir_accel[6];

		/// Direction code for querying specific acceleration in direction. Positive and negative directions for each axis.
		enum Direction{NX, PX, NY, PY, NZ, PZ};

		/// \brief Queries acceleration toward the given direction, taking anisotropic acceleration into account.
		double getAccel(Direction)const;

		Vec3d getAccelVec(const Vec3d &velo)const;
	};
protected:
	virtual void init();
	virtual HitBoxList *getTraceHitBoxes()const;


	/// \brief A class that processes maneuver parameters.
	class EXPORT ManeuverParamsProcess : public SqInitProcess{
	public:
		ManeuverParams &mn;
		ManeuverParamsProcess(ManeuverParams &mn) : mn(mn){}
		virtual void process(HSQUIRRELVM)const;
	};

	struct EnginePos{ Vec3d pos; Quatd rot; };
	typedef std::vector<EnginePos> EnginePosList;
	class EXPORT EnginePosListProcess : public SqInitProcess{
	public:
		EnginePosList &vec;
		const SQChar *name;
		bool mandatory;
		EnginePosListProcess(EnginePosList &vec, const SQChar *name, bool mandatory = true) : vec(vec), name(name), mandatory(mandatory){}
		virtual void process(HSQUIRRELVM)const;
	};

	/// Items in this object's inventory
	InventoryItemList inventory;

private:
	static const ManeuverParams mymn;
};

EXPORT void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g);
#ifdef NDEBUG
#define hitbox_draw
#else
void hitbox_draw(const Entity *pt, const double sc[3], int hitflags = 0);
#endif

void space_collide(Entity *pt, WarSpace *w, double dt, Entity *collideignore, Entity *collideignore2);


#endif
