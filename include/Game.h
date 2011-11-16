#ifndef GAME_H
#define GAME_H
#include "Player.h"
#include "Universe.h"
#include "export.h"
#include "serial.h"
/** \file
 * \brief Defines Game class
 */

class select_box_callback;

class SerializeStream;
class UnserializeStream;

/// \brief The object that stores everything in a game.
class EXPORT Game{
protected:
	static void(*serverInits[])(Game&);
	static int nserverInits;
public:
	Player *player;
	Universe *universe;
	double &flypower;

	Game() : player(new Player()), universe(new Universe(player)), flypower(player->freelook->flypower){
		player->cs = universe;
	}

	~Game(){
		delete player;
		delete universe;
	}

	void lightOn();
	void draw_func(Viewer &vw, double dt);
	void draw_gear(double dt);
	void drawindics(Viewer *vw);
	void display_func(void);
	void clientDraw(double gametime, double dt);
	bool select_box(double x0, double x1, double y0, double y1, const Mat4d &rot, unsigned flags, select_box_callback *sbc);
	void mouse_func(int button, int state, int x, int y);

	virtual void serialize(SerializeStream &ss);
	virtual void unserialize(UnserializeContext &usc);

	void anim(double dt);

	static void addServerInits(void (*f)(Game &));
};

/// \brief Game for the server.
class ServerGame : public Game{
public:
	/// \brief Server constructor executes initializers
	ServerGame(){
		for(int i = 0; i < nserverInits; i++)
			serverInits[i](*this);
	}
};

#endif
