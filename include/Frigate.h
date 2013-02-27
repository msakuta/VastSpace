#ifndef FRIGATE_H
#define FRIGATE_H
#include "Warpable.h"
#include "shield.h"

class EXPORT Frigate : public Warpable{
public:
	typedef Warpable st;
protected:
	ShieldEffect se;
	double shieldAmount;
	double shieldGenSpeed; // To make shield generation inertial.
	Docker *mother;
	int paradec;

	Frigate(Game *game) : st(game){}
	Frigate(WarField *);
	void drawShield(wardraw_t *wd);
	bool cull(wardraw_t *);
	Entity *findMother();

	static const ManeuverParams frigate_mn;
public:
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual double getHitRadius()const;
	virtual double shieldRadius()const;
	virtual int takedamage(double damage, int hitpart);
	virtual void onBulletHit(const Bullet *, int hitpart);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retnormal);
	virtual Props props()const;
	virtual int popupMenu(PopupMenu &list);
	virtual const ManeuverParams &getManeuve()const;
	virtual double maxenergy()const;
	virtual double maxshield()const;
	virtual Dockable *toDockable();
	virtual bool solid(const Entity *)const;
	virtual bool command(EntityCommand *);
	static hitbox hitboxes[];
	static const int nhitboxes;
	static const double modelScale;
protected:
	short bbodyMask()const;
};

#endif
