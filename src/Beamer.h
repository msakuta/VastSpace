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
//	struct tent3d_fpol *pf[1];
//	scarry_t *dock;
	float undocktime;
	static GLuint disp;
//	static const double sufscale;
public:
	Beamer(){init();}
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
	virtual void enterField(WarField *target);
	virtual void leaveField(WarField *w);
	virtual void anim(double);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(WarDraw *);
	virtual double maxhealth()const;
	virtual Props props()const;
	virtual bool undock(Docker*);
	static void cache_bridge(void);
	static Entity *create(WarField *w, Builder *);
//	static const Builder::BuildStatic builds;
};

#endif
