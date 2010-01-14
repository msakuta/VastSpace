#ifndef WARPABLE_H
#define WARPABLE_H

#include "entity.h"
#include "coordsys.h"
#include "war.h"
#include "shield.h"
extern "C"{
#include <clib/avec3.h>
#include <clib/suf/sufdraw.h>
}

struct hitbox;

/* List of spaceship tasks that all types of ship share. */
enum sship_task{
	sship_idle, /* Approach to stationary orbit; relative velocity zero against local coordsys. */
	sship_undock, /* Undocked and exiting */
	sship_undockque, /* Queue in a line of undock waiting. */
	sship_dock, /* Docking */
	sship_dockque, /* Queue in a line of dock waiting. */
	sship_moveto, /* Move to indicated location. */
	sship_parade, /* Parade formation. */
	sship_attack, /* Attack to some object. */
	sship_away, /* Fighters going away in attacking cycle. */
	sship_gather, /* Gather resources. */
	sship_harvest, /* Harvest resources. */
	sship_harvestreturn, /* Harvest resources. */
	num_sship_task
};


class Warpable : public virtual Entity{
public:
	typedef Entity st;
	Vec3d warpdst;
	double warpSpeed;
	double totalWarpDist, currentWarpDist;
	double capacitor; /* Temporarily stored energy, MegaJoules */
	int warping;
	CoordSys *warpcs, *warpdstcs;
//	WarField *warp_next_warf;

	Warpable(){}
	Warpable(WarField *w);

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void anim(double dt);
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

	void maneuver(const amat4_t mat, double dt, const struct maneuve *mn);
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
private:
	static const maneuve mymn;
};

void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g);
#ifdef NDEBUG
#define hitbox_draw
#else
void hitbox_draw(const Entity *pt, const double sc[3]);
#endif
suf_t *CallLoadSUF(const char *fname);

void space_collide(Entity *pt, WarSpace *w, double dt, Entity *collideignore, Entity *collideignore2);

#endif
