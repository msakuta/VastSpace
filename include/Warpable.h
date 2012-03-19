#ifndef WARPABLE_H
#define WARPABLE_H

#include "Entity.h"
#include "CoordSys.h"
#include "war.h"
#include "shield.h"
#ifndef DEDICATED
#include <gl/GL.h>
#else
typedef unsigned int GLuint;
#endif


struct hitbox;
struct hardpoint_static;

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


class EXPORT Warpable : public Entity{
public:
	typedef Entity st;
	Vec3d dest; // Move order destination
	Vec3d warpdst;
	double warpSpeed;
	double totalWarpDist, currentWarpDist;
	double capacitor; /* Temporarily stored energy, MegaJoules */
	int warping;
	CoordSys *warpcs, *warpdstcs;
//	WarField *warp_next_warf;
	enum sship_task task;
	int direction;

	Warpable(){}
	Warpable(WarField *w);

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void anim(double dt);
	virtual void drawtra(wardraw_t *);
	virtual void drawHUD(wardraw_t *);
	void control(input_t *, double);
	unsigned analog_mask();
	virtual int popupMenu(PopupMenu &list);
	virtual Warpable *toWarpable();
	virtual Props props()const;
	struct maneuve;
	virtual const maneuve &getManeuve()const;
	virtual double maxenergy()const = 0;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual bool command(EntityCommand *com);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void post_warp();

	virtual short getDefaultCollisionMask()const;

	void maneuver(const Mat4d &mat, double dt, const struct maneuve *mn);
	void steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void warp_collapse();
	void drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale);

	struct maneuve{
		double accel;
		double maxspeed;
		double angleaccel;
		double maxanglespeed;
		double capacity; /* capacity of capacitor [MJ] */
		double capacitor_gen; /* generated energy [MW] */
	};
protected:
	virtual void init();

	class SqInitProcess{
	public:
		virtual void process(HSQUIRRELVM v) = 0;
	};

	/// \brief Initializes hitboxes and hardpoints by Squirrel script.
	bool sq_init(const SQChar *scriptFile, std::vector<SqInitProcess*> &procs);

	class HitboxProcess : public SqInitProcess{
	public:
		std::vector<hitbox> &hitboxes;
		HitboxProcess(std::vector<hitbox> &hitboxes) : hitboxes(hitboxes){}
		virtual void process(HSQUIRRELVM);
	};

	class HardPointProcess : public SqInitProcess{
	public:
		std::vector<hardpoint_static> &hardpoints;
		HardPointProcess(std::vector<hardpoint_static> &hardpoints) : hardpoints(hardpoints){}
		virtual void process(HSQUIRRELVM);
	};

	class DrawOverlayProcess : public SqInitProcess{
	public:
		GLuint &disp;
		DrawOverlayProcess(GLuint &disp) : disp(disp){}
		virtual void process(HSQUIRRELVM v);
	};

	/// \brief Static information for navigation lights.
	struct Navlight{
		Vec3d pos; ///< Position of this navlight relative to origin of the Entity.
		Vec4f color; ///< Color values in RGBA
		float radius; ///< Apparent radius of the light.
		float period; ///< Period of light cycle
		float phase; ///< Phase offset in the period
	};

	class NavlightsProcess : public SqInitProcess{
	public:
		std::vector<Navlight> &navlights;
		NavlightsProcess(std::vector<Navlight> &navlights) : navlights(navlights){}
		virtual void process(HSQUIRRELVM v);
	};
private:
	static const maneuve mymn;
};

EXPORT void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g);
#ifdef NDEBUG
#define hitbox_draw
#else
void hitbox_draw(const Entity *pt, const double sc[3], int hitflags = 0);
#endif

void space_collide(Entity *pt, WarSpace *w, double dt, Entity *collideignore, Entity *collideignore2);

#endif
