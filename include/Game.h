#ifndef GAME_H
#define GAME_H
#include "Player.h"
#include "Universe.h"
#include "export.h"
#include "serial.h"
/** \file
 * \brief Defines Game class
 */

class ClientGame;

class select_box_callback;

class SerializeStream;
class UnserializeStream;

/// \brief The state vector object that is shared among server and client Squirrel VM.
///
/// For security reasons, we do not allow the network stream to transmit arbitrary code of Squirrel,
/// but only a dictionary of variables.
class SquirrelShare : public Serializable, public Observable{
public:
	SquirrelShare(Game *game = NULL) : Serializable(game){}

	std::map<dstring, dstring> dict;

	static const unsigned classId;
	const char *classname()const;

	/// Creates and pushes an SquirrelShare object to Squirrel stack.
	static void sq_pushobj(HSQUIRRELVM v, SquirrelShare *e);

	/// Returns an SquirrelShare object being pointed to by an object in Squirrel stack.
	static SquirrelShare *sq_refobj(HSQUIRRELVM v, SQInteger idx = 1);

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
	typedef void (*ServerInitFunc)(Game &);
	typedef std::vector<ServerInitFunc> ServerInitList;
	static ServerInitList &serverInits();

	typedef void (*ClientInitFunc)(ClientGame &);
	typedef std::vector<ClientInitFunc> ClientInitList;
	static ClientInitList &clientInits();
public:
	typedef std::vector<Player*> PlayerList;
	Player *player;
	PlayerList players;
	Universe *universe;
	double flypower()const;
	HSQUIRRELVM sqvm;

	Game() : player(NULL), universe(NULL), sqvm(NULL), sqshare(NULL), idGenerator(1), clientDeleting(false){
	}

	virtual ~Game(){
		delete player;
		delete universe;
	}

	void lightOn(Viewer &vw);
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
	IdMap &idmap();

	virtual void serialize(SerializeStream &ss);
	virtual void unserialize(UnserializeContext &usc);

	virtual void anim(double dt) = 0;

	static void addServerInits(ServerInitFunc f);
	static void addClientInits(ClientInitFunc f);

	void sq_replaceUniverse(Universe *);
	void sq_replacePlayer(Player *);

	/// \brief Returns whether this Game object is server game.
	///
	/// Be careful that !isServer() not always equal to isClient().
	/// Standalone game can be both server and client.
	///
	/// Default value is provided in this abstract class to prevent pure calls in the destructor.
	/// \sa isClient()
	virtual bool isServer()const{return false;}

	/// \brief Returns whether this Game object is client game.
	///
	/// Be careful that !isClient() not always equal to isServer().
	/// Standalone game can be both server and client.
	///
	/// Default value is provided in this abstract class to prevent pure calls in the destructor.
	/// \sa isServer()
	virtual bool isClient()const{return false;}

	virtual bool isRawCreateMode()const;

	void setSquirrelShare(SquirrelShare *p);
	SquirrelShare *getSquirrelShare(){return sqshare;}
	const SquirrelShare *getSquirrelShare()const{return sqshare;}

	/// \brief Fetch the next Serializable id in this Game object's environment.
	virtual Serializable::Id nextId();

	/// \brief Loads a Stellar Structure Definition file (.ssd) of a given name.
	int StellarFileLoad(const char *fname);

	static SQInteger sqf_addent(HSQUIRRELVM v);

	typedef std::vector<SerializableId> DeleteQue;
	DeleteQue &getDeleteQue(){return deleteque;}
	bool isClientDeleting()const{return clientDeleting;}

	typedef ObservableSet<Entity> ObjSet;
	virtual ObjSet *getClientObjSet(){return NULL;}

	virtual void beginLoadingSection(){}
	virtual void endLoadingSection(){}

protected:
	IdMap idunmap;
	SquirrelShare *sqshare;
	Serializable::Id idGenerator;
	DeleteQue deleteque;
	bool clientDeleting;

	friend class StellarContext;

	void adjustAutoExposure(Viewer &vw);
};


#ifndef DEDICATED
class ClientGame : public virtual Game{
public:
	ClientGame();
	virtual void anim(double dt);
	virtual bool isServer()const{return false;}
	virtual bool isClient()const{return true;}
	virtual Serializable::Id nextId();
	virtual bool isRawCreateMode()const;
	virtual ObjSet *getClientObjSet(){return &clientObjs;}
	virtual void beginLoadingSection(){loading = true;}
	virtual void endLoadingSection(){loading = false;}
protected:
	ObjSet clientObjs;
	bool loading;
	void clientUpdate(double dt);
};
#endif

/// \brief Game for the server.
class ServerGame : public virtual Game{
public:
	/// \brief Server constructor executes initializers
	ServerGame();
	void init();
	virtual void anim(double dt);
	virtual bool isServer()const{return true;}
	virtual bool isClient()const{return false;}
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
