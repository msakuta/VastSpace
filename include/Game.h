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
class SquirrelBind : public Serializable, public Observable{
public:
	SquirrelBind(Game *game = NULL) : Serializable(game){}

	std::map<dstring, dstring> dict;

	static const unsigned classId;
	const char *classname()const;

	/// Creates and pushes an SquirrelBind object to Squirrel stack.
	static void sq_pushobj(HSQUIRRELVM v, SquirrelBind *e);

	/// Returns an SquirrelBind object being pointed to by an object in Squirrel stack.
	static SquirrelBind *sq_refobj(HSQUIRRELVM v, SQInteger idx = 1);

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

	Game() : player(NULL), universe(NULL), sqvm(NULL), sqbind(NULL), idGenerator(1){
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
	void clientDraw(double gametime, double dt);
	bool select_box(double x0, double x1, double y0, double y1, const Mat4d &rot, unsigned flags, select_box_callback *sbc);
	void mouse_func(int button, int state, int x, int y);

	void idUnmap(UnserializeContext &sc);
	void idUnserialize(UnserializeContext &usc);

//	typedef std::map<unsigned long, Serializable*> IdMap;
	typedef UnserializeMap IdMap;
	const IdMap &idmap()const;

	virtual void serialize(SerializeStream &ss);
	virtual void unserialize(UnserializeContext &usc);

	virtual void anim(double dt) = 0;
	virtual void postframe() = 0;

	static void addServerInits(void (*f)(Game &));

	void sq_replaceUniverse(Universe *);
	void sq_replacePlayer(Player *);

	virtual bool isServer()const;
	virtual bool isRawCreateMode()const;

	void setSquirrelBind(SquirrelBind *p);
	SquirrelBind *getSquirrelBind(){return sqbind;}
	const SquirrelBind *getSquirrelBind()const{return sqbind;}

	/// \brief Fetch the next Serializable id in this Game object's environment.
	Serializable::Id nextId();

	/// \brief Loads a Stellar Structure Definition file (.ssd) of a given name.
	int StellarFileLoad(const char *fname);

	static SQInteger sqf_addent(HSQUIRRELVM v);

	typedef std::vector<SerializableId> DeleteQue;
	DeleteQue &getDeleteQue(){return deleteque;}
protected:
	IdMap idunmap;
	SquirrelBind *sqbind;
	Serializable::Id idGenerator;
	DeleteQue deleteque;

	int StellarFileLoadInt(const char *fname, CoordSys *root, struct varlist *vl);
	int stellar_coordsys(StellarContext &sc, CoordSys *cs);
};


#ifndef DEDICATED
class ClientGame : public Game{
public:
	ClientGame();
	virtual void anim(double dt);
	virtual void postframe();
};
#endif

/// \brief Game for the server.
class ServerGame : public Game{
public:
	/// \brief Server constructor executes initializers
	ServerGame();
	void init();
	virtual void anim(double dt);
	virtual void postframe();
	virtual bool isServer()const;
	virtual bool isRawCreateMode()const;
protected:
	bool loading; ///< The flag only active when unserialized from a file.
};




//-----------------------------------------------------------------------------
//     Inline Implementation
//-----------------------------------------------------------------------------

inline double Game::flypower()const{
	if(!player)
		return 0;
	else
		return player->freelook->flypower;
}

inline Serializable::Id Game::nextId(){
	return idGenerator++;
}

#endif
