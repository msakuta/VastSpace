/** \file
 * \brief Definition of Warpable class.
 */
#ifndef WARPABLE_H
#define WARPABLE_H

#include "Autonomous.h"
#include "CoordSys.h"
#include "war.h"


/// \brief An autonomous object that can use warp drives.
class EXPORT Warpable : public Autonomous{
public:
	typedef Autonomous st;
	Vec3d warpdst;
	double warpSpeed;
	double totalWarpDist, currentWarpDist;
	double capacitor; /* Temporarily stored energy, MegaJoules */
	int warping;
	WeakPtr<CoordSys> warpcs;
	WeakPtr<CoordSys> warpdstcs;

	Warpable(Game *game) : st(game){}
	Warpable(WarField *w);

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void enterField(WarField*);
	virtual void anim(double dt);
	virtual void drawHUD(wardraw_t *);
	virtual Warpable *toWarpable();
	virtual Props props()const;
	virtual double maxenergy()const = 0;
	virtual bool command(EntityCommand *com);
	virtual void post_warp();
	virtual double warpCostFactor()const;

	virtual Vec3d absvelo()const;

	void maneuver(const Mat4d &mat, double dt, const ManeuverParams *mn);
	void warp_collapse();
	void drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale);

protected:
	virtual void init();

	static HSQOBJECT sqPopupMenu;
	HSQOBJECT getSqPopupMenu()override;

	virtual SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const;
	virtual SQInteger sqSet(HSQUIRRELVM v, const SQChar *name);
};


#endif
