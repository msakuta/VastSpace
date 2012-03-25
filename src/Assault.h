/** \file
 * \brief Defines Assault class.
 */
#ifndef ASSAULT_H
#define ASSAULT_H
#include "Frigate.h"
#include "arms.h"
#include "Docker.h"

/// Medium class ship with replaceable turrets.
class Assault : public Frigate{
protected:
	float engineHeat;
	Assault *formPrev;
	static suf_t *sufbase;
	ArmBase **turrets;

public:
	typedef Frigate st;
	Assault(Game *game) : st(game){init();}
	Assault(WarField *w);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	static const unsigned classid;
	static EntityRegister<Assault> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void anim(double);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int index);
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	friend class GLWarms;
	static Entity *create(WarField *w, Builder *);
//	static const Builder::BuildStatic builds;
protected:
	bool buildBody();
	static std::vector<hardpoint_static*> hardpoints;
	static GLuint disp;
	static std::vector<hitbox> hitboxes;
	static std::vector<Navlight> navlights;
};

int cmd_armswindow(int argc, char *argv[], void *pv);

#endif
