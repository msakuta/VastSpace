/** \file
 * \brief Definition of Universe class, root of all coordinate systems. */
#ifndef UNIVERSE_H
#define UNIVERSE_H
#include "CoordSys.h"

class Game;

/// Class that is only one in the game.
class Universe : public CoordSys{
public:
	typedef CoordSys st;
	double timescale;
	double global_time;
	double astro_time;
	double gravityfactor;
	double astro_timescale; ///< The timescale for astronomical objects, used for debugging celestial motion.
	bool paused;
	static const unsigned version; ///< Saved file version, checked on loading

	Universe(Game *game);
	Universe(const char *path, CoordSys *root) : st(path, root){}
	~Universe();
	const Static &getStatic()const{return classRegister;}
	static ClassRegister<Universe> classRegister;
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void clientUpdate(double dt);
	void draw(const Viewer *);
	virtual Universe *toUniverse(){return this;}
	void csUnserialize(UnserializeContext &usc);
	void csUnmap(UnserializeContext &);
	Game *getGame();
	static int cmd_save(int argc, char *argv[], void *pv);
	static int cmd_load(int argc, char *argv[], void *pv);
	static bool sq_define(HSQUIRRELVM);
	static double getGravityFactor(const CoordSys *);
protected:
	static SQInteger sqf_get(HSQUIRRELVM);
	static SQInteger sqf_set(HSQUIRRELVM);
};


inline Game *Universe::getGame(){
	return game;
}

/// \brief Returns Universal Gravitational Constant from any node of a CoordSys tree.
///
/// Returns 1.0 if the Universe object cannot be obtained.
inline double Universe::getGravityFactor(const CoordSys *cs){
	const CoordSys *root = cs->findcspath("/");
	if(!root)
		return 1.;
	const Universe *u = root->toUniverse();
	if(!u)
		return 1.;
	return u->gravityfactor;
}

#endif
