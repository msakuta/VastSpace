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

/// \brief The Squirrel binding entity to share Squirrel state variables among server and clients.
///
/// For security reasons, we do not allow the network stream to transmit arbitrary code of Squirrel,
/// but only a dictionary of variables.
class SquirrelBind : public Serializable{
public:
	std::map<dstring, dstring> dict;

	static const unsigned classId;
	const char *classname()const;

	/// Define class Player for Squirrel.
	static void sq_define(HSQUIRRELVM v);

	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_set(HSQUIRRELVM v);

protected:
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);

};


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
	HSQUIRRELVM sqvm;

	Game() : player(NULL), universe(NULL), sqvm(NULL), sqbind(NULL){
	}

	~Game(){
		delete player;
		delete universe;
	}

	void lightOn();
	void draw_func(Viewer &vw, double dt);
	void draw_gear(double dt);
	void drawindics(Viewer *vw);
	virtual void init();
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

	void sq_replacePlayer(Player *);

	virtual bool isServer()const;

	void setSquirrelBind(SquirrelBind *p);
	SquirrelBind *getSquirrelBind(){return sqbind;}
	const SquirrelBind *getSquirrelBind()const{return sqbind;}

protected:
	IdMap idunmap;
	SquirrelBind *sqbind;
};

/// \brief Game for the server.
class ServerGame : public Game{
public:
	/// \brief Server constructor executes initializers
	ServerGame();
	void init();
	virtual bool isServer()const;
};


inline double Game::flypower()const{
	if(!player)
		return 0;
	else
		return player->freelook->flypower;
}

#endif
