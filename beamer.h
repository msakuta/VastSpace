#ifndef SCARRY_H
#define SCARRY_H

#include "entity.h"
#include "coordsys.h"
#include "war.h"
//#include "mturret.h"
//#include "bhole.h"
extern "C"{
#include <clib/avec3.h>
#include <clib/suf/sufdraw.h>
}

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

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
	avec3_t warpdst;
	double warpSpeed;
	double totalWarpDist, currentWarpDist;
	double capacitor; /* Temporarily stored energy, MegaJoules */
	int warping;
	CoordSys *warpcs, *warpdstcs;
	WarField *warp_next_warf;

	Warpable();

	void control(input_t *, double);
	unsigned analog_mask();
	void maneuver(const amat4_t mat, double dt, const struct maneuve *mn);
};

class Beamer : public Warpable{
public:
	typedef Warpable st;
protected:
	struct shieldWavelet *sw;
	double shieldAmount;
	double shield;
	double charge;
	avec3_t integral;
	double beamlen;
	double cooldown;
	struct tent3d_fpol *pf[1];
//	scarry_t *dock;
	float undocktime;
	static suf_t *sufbase;
public:
	Beamer();
	Beamer(WarField *w);
	void anim(double);
	void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
};


#if 0
struct scarry{
	warpable_t st;
	mturret_t turrets[10];
	double ru;
	double baycool;
	double cargo; /* amount of cargohold [m^3] */
	double build;
	struct armscustom{
		const struct build_data *builder;
		int c;
		struct arms *a;
	} *armscustom;
	int narmscustom;
	const struct build_data *builder, *buildque[SCARRY_BUILDQUESIZE];
	struct arms *buildarm, *buildarms[SCARRY_BUILDQUESIZE];
	int buildquenum[SCARRY_BUILDQUESIZE];
	int buildque0, buildque1; /* start and end pointer in buildque */
	int nbuildque; /* count of entries in buildqueue, for start/end pointer method is unable to handle the case of nbuildque == BUILDQUESIZE */
	int paradec; /* parade position counter */
	int undockc; /* undock counter */
	int remainDocked;
	bhole_t bholes[50];
	bhole_t *frei;
	sufdecal_t *sd;
	entity_t *dock; /* docked entity list */
	const char *name; /* unique name automatically assigned */
/*	struct tent3d_fpol *pf[4];*/
};

/* Active Radar or Hyperspace Sonar.
   AR costs less energy but has limitation in speed (light), while
  HS propagates on hyperspace field where limitation of speed do not exist.
*/
struct hypersonar{
	avec3_t pos;
	double life;
	double rad;
	double speed;
	struct coordsys *cs;
	struct hypersonar *next;
	int type; /* 0 - Active Radar, 1 - Hyperspace Sonar. */
} *g_hsonar;
#endif

#endif
