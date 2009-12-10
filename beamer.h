#ifndef BEAMER_H
#define BEAMER_H

#include "entity.h"
#include "coordsys.h"
#include "war.h"
#include "arms.h"
#include "shield.h"
extern "C"{
#include <clib/avec3.h>
#include <clib/suf/sufdraw.h>
}

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

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


struct maneuve;
class Warpable : public Entity{
public:
	typedef Entity st;
	Vec3d warpdst;
	double warpSpeed;
	double totalWarpDist, currentWarpDist;
	double capacitor; /* Temporarily stored energy, MegaJoules */
	int warping;
	CoordSys *warpcs, *warpdstcs;
//	WarField *warp_next_warf;

	Warpable(WarField *w);

	virtual void anim(double dt);
	void control(input_t *, double);
	unsigned analog_mask();
	virtual int popupMenu(char ***const titles, int **keys, char ***cmds, int *num);
	virtual int popupMenu(PopupMenuItem **list);
	virtual Warpable *toWarpable();
	virtual Props props()const;
	struct maneuve;
	virtual const maneuve &getManeuve()const;
	virtual double maxenergy()const = 0;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;

	void maneuver(const amat4_t mat, double dt, const struct maneuve *mn);
	void warp_collapse();

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

class Frigate : public Warpable{
public:
	typedef Warpable st;
protected:
	ShieldEffect se;
	double shieldAmount;
	Frigate(WarField *);
	void drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos);
	void drawShield(wardraw_t *wd);

public:
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual double hitradius();
	virtual int takedamage(double damage, int hitpart);
	virtual void bullethit(const Bullet *);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual std::vector<cpplib::dstring> props()const;
	virtual const maneuve &getManeuve()const;
	virtual double maxenergy()const, maxshield()const;
	static hitbox hitboxes[];
	static const int nhitboxes;
};

class Beamer : public Frigate{
public:
	typedef Frigate st;
protected:
	double charge;
	Vec3d integral;
	double beamlen;
	double cooldown;
//	struct tent3d_fpol *pf[1];
//	scarry_t *dock;
	float undocktime;
	static suf_t *sufbase;
	static const double sufscale;
public:
	Beamer(WarField *w);
	const char *idname()const;
	const char *classname()const;
	virtual void anim(double);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual std::vector<cpplib::dstring> props()const;
	static void cache_bridge(void);
};

class Assault : public Frigate{
protected:
	static suf_t *sufbase;
	union{
		struct{
			ArmBase *turret0, *turret1, *turret2, *turret3;
		};
		ArmBase *turrets[4];
	};
	static const hardpoint_static hardpoints[];
public:
	typedef Frigate st;
	Assault(WarField *w);
	const char *idname()const;
	const char *classname()const;
	virtual void anim(double);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual const ArmBase *armsGet(int index)const;
	virtual void attack(Entity *target);
	friend class GLWarms;
};

int cmd_armswindow(int argc, char *argv[], void *pv);

void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g);
#ifdef NDEBUG
#define hitbox_draw
#else
void hitbox_draw(const Entity *pt, const double sc[3]);
#endif
suf_t *CallLoadSUF(const char *fname);

GLuint CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2);

#endif
