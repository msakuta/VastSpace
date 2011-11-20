/** \file
 * \brief Definition of Universe class, root of all coordinate systems. */
#ifndef UNIVERSE_H
#define UNIVERSE_H
#include "CoordSys.h"

/// Class that is only one in the game.
class Universe : public CoordSys{
public:
	typedef CoordSys st;
	double timescale;
	double global_time;
	double astro_time;
	Player *ppl;
	bool paused;
	static const unsigned version; ///< Saved file version, checked on loading

	Universe(Player *pl);
	Universe(){}
	Universe(const char *path, CoordSys *root) : st(path, root){}
	~Universe();
	const Static &getStatic()const{return classRegister;}
	static ClassRegister<Universe> classRegister;
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void draw(const Viewer *);
	virtual Universe *toUniverse(){return this;}
	void csUnserialize(UnserializeContext &usc);
	void csUnmap(UnserializeContext &);
	static int cmd_save(int argc, char *argv[], void *pv);
	static int cmd_load(int argc, char *argv[], void *pv);
	static bool sq_define(HSQUIRRELVM);
protected:
	static SQInteger sqf_get(HSQUIRRELVM);
};

#endif
