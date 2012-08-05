#ifndef BATTLESHIP_H
#define BATTLESHIP_H
#include "Warpable.h"
#include "arms.h"

class Battleship : public Warpable{
protected:
	float engineHeat;
	ArmBase **turrets;
	static std::vector<hardpoint_static*> hardpoints;
	static std::vector<hitbox> hitboxes;
	static double modelScale;
	static GLuint disp;
	static std::vector<Navlight> navlights;
public:
	typedef Warpable st;
	Battleship(Game *game) : st(game){init();}
	Battleship(WarField *w);
	static const unsigned classid;
	static EntityRegister<Battleship> entityRegister;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double getHitRadius()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe();
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *wd);
	virtual void drawtra(wardraw_t *wd);
	virtual void drawOverlay(wardraw_t *);
	virtual int takedamage(double damage, int hitpart);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int index);
	virtual int popupMenu(PopupMenu &list);
	virtual bool command(EntityCommand *com);
	virtual double maxenergy()const;
	const ManeuverParams &getManeuve()const;
protected:
	bool buildBody();
	void static_init();
	virtual void init();
};

#endif
