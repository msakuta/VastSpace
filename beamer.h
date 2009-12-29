#ifndef BEAMER_H
#define BEAMER_H
#include "Frigate.h"

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
	static suf_t *sufbase;
	static const double sufscale;
public:
	Beamer(){}
	Beamer(WarField *w);
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double maxhealth()const;
	virtual std::vector<cpplib::dstring> props()const;
	static void cache_bridge(void);
};

#endif
