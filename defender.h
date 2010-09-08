/** \file
 * \brief Declaration of Defender, a space fighter that sit tight and guard settlements.
 *
 * Also declares DeployCommand, acceptable by Defender.
 */
#ifndef DEFENDER_H
#define DEFENDER_H

#include "beamer.h"
#include "EntityCommand.h"
#include "coordsys.h"
#include "war.h"
#include "arms.h"
#include "shield.h"
extern "C"{
#include <clib/avec3.h>
#include <clib/suf/sufdraw.h>
}

#define PIDAIM_PROFILE 0

/// Space Interceptor (small fighter)
class Defender : public Entity{
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
		Deploy,
		NumTask
	};
/*	enum Attitude{
		Passive,
		Aggressive
	};*/
	Vec3d aac; /* angular acceleration */
	double thrusts[3][2]; /* 3 pairs of thrusters, 2 directions each */
	double throttle;
	double fuel;
	double cooldown;
	Vec3d dest;
	float fcloak;
	float heat;
	struct tent3d_fpol *pf[4]; ///< Trailing smoke
	Docker *mother; ///< Mother ship that will be returned to when out of fuel
	int hitsound;
	int paradec;
	Task task;
	bool docked, returning, away, cloak, forcedEnemy;
	float reverser; ///< Thrust reverser position, approaches to 1 when throttle is negative.
	float mf; ///< trivial muzzle flashes
	float integral[2]; ///< integration of pitch-yaw space of relative target position
	float fdeploy; ///< Factor of deployment
//	Sceptor *formPrev; ///< previous member in the formation
//	Attitude attitude;

	static const avec3_t gunPos[2];
	void shoot(double dt);
	bool findEnemy(); // Finds the nearest enemy
	void steerArrival(double dt, const Vec3d &target, const Vec3d &targetvelo, double speedfactor = 5., double minspeed = 0.);
	bool cull(Viewer &)const;
	double nlipsFactor(Viewer &)const;
	Entity *findMother();
	static double reloadTime(){return 5.;} ///< Time between shoots
	static double bulletSpeed(){return 5.;} ///< Velocity of projectile shot
	static double modelScale(){return 1./10000.;} ///< Model scale
public:
	Defender();
	Defender(WarField *aw);
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
	virtual void drawtra(wardraw_t *w);
	virtual bool solid(const Entity*)const;
	virtual int takedamage(double damage, int hitpart);
	virtual void postframe();
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
//	virtual Dockable *toDockable();
	virtual double hitradius()const;
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual int popupMenu(PopupMenu &);
	virtual Props props()const;
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	virtual double maxfuel()const;
	static hitbox hitboxes[];
	static const int nhitboxes;
	static void smokedraw(const struct tent3d_line_callback *p, const struct tent3d_line_drawdata *dd, void *private_data);
private:
};

/// \brief Command that tells a Defender to toggle deployment.
///
/// Deployed Defender has better performance over combat, but cannot move forward nor desired directions.
DERIVE_COMMAND(DeployCommand, EntityCommand);


#endif
