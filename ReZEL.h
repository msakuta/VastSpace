#ifndef GUNDAM_REZEL_H
#define GUNDAM_REZEL_H

#include "Frigate.h"
#include "EntityCommand.h"


/// ReZEL
class ReZEL : public Entity{
public:
	typedef Entity st;
protected:
	enum Task{
		Idle = sship_idle,
		Undock = sship_undock,
		Undockque = sship_undockque,
		Dock = sship_dock,
		Dockque = sship_dockque,
		Moveto = sship_moveto,
		Parade = sship_parade,
		DeltaFormation = sship_delta,
		Attack = sship_attack,
		Away = sship_away,
		Auto, // Automatically targets near enemy or parade with mother.
		num_sceptor_task
	};
	enum Attitude{
		Passive,
		Aggressive
	};
	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	double cooldown;
	Vec3d dest;
	float fcloak;
	float heat;
	float fwaverider; ///< Phase of transformation to wave rider
	float fweapon; ///< Phase of weapon switch
	struct tent3d_fpol *pf; ///< Trailing smoke
	Docker *mother; ///< Mother ship that will be returned to when out of fuel
	int hitsound;
	int paradec;
	int magazine; ///< remaining bullet count in the magazine to shoot before reloading
	int weapon; ///< Armed weapon id
	Task task;
	bool docked, returning, away, cloak, forcedEnemy;
	bool waverider;
	float reverser; ///< Thrust reverser position, approaches to 1 when throttle is negative.
	float mf; ///< trivial muzzle flashes
	float integral[2]; ///< integration of pitch-yaw space of relative target position
	ReZEL *formPrev; ///< previous member in the formation
	Attitude attitude;

	static const avec3_t gunPos[2];
	void shootDualGun(double dt);
	bool findEnemy(); // Finds the nearest enemy
	void steerArrival(double dt, const Vec3d &target, const Vec3d &targetvelo, double speedfactor = 5., double minspeed = 0.);
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Entity *findMother();
public:
	ReZEL();
	ReZEL(WarField *aw);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid, entityid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double maxhealth()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void enterField(WarField *);
	virtual void anim(double dt);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual bool solid(const Entity*)const;
	virtual int takedamage(double damage, int hitpart);
	virtual void postframe();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual Dockable *toDockable();
	virtual double hitradius()const;
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int popupMenu(PopupMenu &);
	virtual Props props()const;
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	virtual double maxfuel()const;
	static hitbox hitboxes[];
	static const int nhitboxes;
//	static Entity *create(WarField *w, Builder *mother);
	static int cmd_dock(int argc, char *argv[], void *);
	static int cmd_parade_formation(int argc, char *argv[], void*);
	static void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);
	static double pid_ifactor;
	static double pid_pfactor;
	static double pid_dfactor;
private:
	Vec3d evelo;
#if PIDAIM_PROFILE
	Vec3d epos;
	Vec3d iepos;
#endif
};


struct TransformCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	TransformCommand(){}
	TransformCommand(HSQUIRRELVM v, Entity &e);
	int formid;
};

struct WeaponCommand : public EntityCommand{
	typedef EntityCommand st;
	static int construction_dummy;
	static EntityCommandID sid;
	virtual EntityCommandID id()const;
	virtual bool derived(EntityCommandID)const;
	WeaponCommand(){}
	WeaponCommand(HSQUIRRELVM v, Entity &e);
	int weaponid;
};

#endif
