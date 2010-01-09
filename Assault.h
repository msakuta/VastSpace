#ifndef ASSAULT_H
#define ASSAULT_H
#include "Frigate.h"
#include "arms.h"
#include "Scarry.h"

class Assault : public Frigate{
protected:
	static suf_t *sufbase;
	ArmBase **turrets;
	static hardpoint_static *hardpoints;
	static int nhardpoints;
public:
	typedef Frigate st;
	Assault(){init();}
	Assault(WarField *w);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual const ArmBase *armsGet(int index)const;
	virtual void attack(Entity *target);
	virtual bool undock(Docker *);
	friend class GLWarms;
	static Entity *create(WarField *w, Builder *);
	static const Builder::BuildStatic builds;
};

int cmd_armswindow(int argc, char *argv[], void *pv);

#endif
