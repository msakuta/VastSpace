#ifndef DESTROYER_H
#define DESTROYER_H
#include "Warpable.h"
#include "arms.h"

class Destroyer : public Warpable{
protected:
	float engineHeat;
	ArmBase **turrets;
	static std::vector<hardpoint_static*> hardpoints;
	static struct hitbox hitboxes[];
	static const int nhitboxes;
	static const double sufscale;
public:
	typedef Warpable st;
	Destroyer(Game *game) : st(game){init();}
	Destroyer(WarField *w);
	static const unsigned classid;
	static EntityRegister<Destroyer> entityRegister;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double hitradius()const;
	virtual void anim(double dt);
	virtual void postframe();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *wd);
	virtual void drawtra(wardraw_t *wd);
	virtual int takedamage(double damage, int hitpart);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int index);
	virtual bool command(EntityCommand *com);
	virtual double maxenergy()const;
	const maneuve &getManeuve()const;
protected:
	void static_init();
	virtual void init();
};

class WireDestroyer : public Warpable{
protected:
	double wirephase;
	double wireomega;
	double wirelength;
	static struct hitbox hitboxes[];
	static const int nhitboxes;
public:
	typedef Warpable st;
	WireDestroyer(Game *game) : st(game){}
	WireDestroyer(WarField *w);
	static const unsigned classid;
	static EntityRegister<WireDestroyer> entityRegister;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double hitradius()const;
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *wd);
	virtual void drawtra(wardraw_t *wd);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	const maneuve &getManeuve()const;
};

#endif
