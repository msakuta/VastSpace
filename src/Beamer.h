/** \file
 * \bried Definition of Beamer class, a frigate with a large beam cannon.
 */
#ifndef BEAMER_H
#define BEAMER_H
#include "Frigate.h"
#include "Docker.h"

class Beamer : public Frigate{
public:
	typedef Frigate st;
protected:
	double charge;
	Vec3d integral;
	double beamlen;
	double cooldown;
//	Tefpol *pf[1];
//	scarry_t *dock;
	float undocktime;
public:
	Beamer(Game *game) : st(game){init();}
	Beamer(WarField *w);
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<Beamer> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool buildBody();
	virtual const char *dispname()const;
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(WarDraw *);
	virtual double maxhealth()const;
	virtual Props props()const;
	virtual bool undock(Docker*);
	virtual ManeuverParams &getManeuve()const;
	static void cache_bridge(void);
	static Entity *create(WarField *w, Builder *);
//	static const Builder::BuildStatic builds;
protected:
	static GLuint disp;
	static double modelScale;
	static double defaultMass;
	static ManeuverParams mn;
	static HitBoxList hitboxes;
	static std::vector<Navlight> navlights;
	std::vector<hitbox> *getTraceHitBoxes()const;
};

#endif
