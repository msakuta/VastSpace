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
	std::vector<Player *> players;
	Universe *universe;
	double flypower()const;
	unsigned char *buf;
	int bufsiz;

	Game() : player(NULL), universe(NULL), buf(NULL){
	}

	~Game(){
		delete player;
		delete universe;
		delete buf;
	}

	void lightOn();
	void draw_func(Viewer &vw, double dt);
	void draw_gear(double dt);
	void drawindics(Viewer *vw);
	void init();
	void display_func(double dt);
	void clientDraw(double gametime, double dt);
	bool select_box(double x0, double x1, double y0, double y1, const Mat4d &rot, unsigned flags, select_box_callback *sbc);
	void mouse_func(int button, int state, int x, int y);

	void idUnmap(UnserializeContext &sc);
	void idUnserialize(UnserializeContext &usc);

	typedef std::map<unsigned long, Serializable*> IdMap;
	const IdMap &idmap()const;

	virtual void serialize(SerializeStream &ss);
	virtual void unserialize(UnserializeContext &usc);

	void anim(double dt);

	static void addServerInits(void (*f)(Game &));

protected:
	IdMap idunmap;
};

/// \brief Game for the server.
class ServerGame : public Game{
public:
	/// \brief Server constructor executes initializers
	ServerGame();
};


inline double Game::flypower()const{
	if(!player)
		return 0;
	else
		return player->freelook->flypower;
}

#endif
