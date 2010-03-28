#ifndef UNIVERSE_H
#define UNIVERSE_H
#include "coordsys.h"


class Universe : public CoordSys{
public:
	typedef CoordSys st;
	double timescale;
	double global_time;
	double astro_time;
	Player *ppl;
	bool paused;
	static const unsigned version; // Saved file version, checked on loading

	Universe(Player *pl) : ppl(pl), paused(true), timescale(1), global_time(0), astro_time(0){
		name = NULL;
		fullname = NULL;
		flags = CS_ISOLATED | CS_EXTENT;
	}
	const char *classname()const;
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void draw(const Viewer *);
	virtual Universe *toUniverse(){return this;}
	void csUnserialize(UnserializeContext &usc);
	void csUnmap(UnserializeContext &);
	static int cmd_save(int argc, char *argv[], void *pv);
	static int cmd_load(int argc, char *argv[], void *pv);
};

#endif
