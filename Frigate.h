#ifndef FRIGATE_H
#define FRIGATE_H
#include "Warpable.h"

class Frigate : public Warpable{
public:
	typedef Warpable st;
protected:
	ShieldEffect se;
	double shieldAmount;
	Frigate(){}
	Frigate(WarField *);
	void drawShield(wardraw_t *wd);

public:
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
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

#endif
