/** \file 
 * \brief Implementation of Game class
 */

#include "antiglut.h"
#include "serial_util.h"
#include "sqadapt.h"
#ifdef _WIN32
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include "Game.h"
extern "C"{
#include <clib/timemeas.h>
}


const unsigned SquirrelShare::classId = registerClass("SquirrelShare", Conster<SquirrelShare>);
const char *SquirrelShare::classname()const{return "SquirrelShare";}
const SQUserPointer tt_SquirrelBind = const_cast<char*>("SquirrelShare");

void SquirrelShare::serialize(SerializeContext &sc){
	sc.o << int(dict.size());
	for(std::map<dstring, dstring>::iterator it = dict.begin(); it != dict.end(); it++)
		sc.o << it->first << it->second;
}

void SquirrelShare::unserialize(UnserializeContext &sc){
	dict.clear();
	int c;
	sc.i >> c;
	for(int i = 0; i < c; i++){
		dstring a, b;
		sc.i >> a;
		sc.i >> b;
		dict[a] = b;
	}
}

/// \brief The release hook of Entity that clears the weak pointer.
///
/// \param size is always 0?
static SQInteger sqh_release(SQUserPointer p, SQInteger size){
	((WeakPtr<SquirrelShare>*)p)->~WeakPtr<SquirrelShare>();
	return 1;
}

void SquirrelShare::sq_pushobj(HSQUIRRELVM v, SquirrelShare *e){
	sq_pushroottable(v);
	// It may seem ineffective to obtain SquirrelShare class from the root table everytime,
	// but we must resolve it anyway, because there can be multiple Squirrel VM.
	// Sharing a dedicated member variables of type HSQOBJECT in Game object is one way,
	// but it couldn't support further classes that would be added to the Game.
	sq_pushstring(v, _SC("SquirrelShare"), -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with SquirrelShare class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError("Something's wrong with SquirrelShare class instance.");
	SQUserPointer p;
	if(SQ_FAILED(sq_getinstanceup(v, -1, &p, NULL)))
		throw SQFError("Something's wrong with Squirrel Class Instace of SquirrelShare.");
	new(p) WeakPtr<SquirrelShare>(e);
	sq_setreleasehook(v, -1, sqh_release);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

SquirrelShare *SquirrelShare::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of Entity.");
	return *(WeakPtr<SquirrelShare>*)up;
}

SQInteger SquirrelShare::sqf_get(HSQUIRRELVM v){
	SquirrelShare *p = sq_refobj(v);
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	std::map<dstring, dstring>::iterator it = p->dict.find(wcs);
	if(it != p->dict.end()){
		sq_pushstring(v, it->second, -1);
		return 1;
	}
	else{
		sq_pushstring(v, _SC(""), -1);
		return 1;
	}
}

SQInteger SquirrelShare::sqf_set(HSQUIRRELVM v){
	SquirrelShare *p = sq_refobj(v);
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(OT_STRING != sq_gettype(v, 3))
		return SQ_ERROR;
	const SQChar *value;
	if(SQ_FAILED(sq_getstring(v, 3, &value)))
		return SQ_ERROR;
	p->dict[wcs] = value;
}

void SquirrelShare::sq_define(HSQUIRRELVM v){
	// Define class SquirrelShare
	sq_pushstring(v, _SC("SquirrelShare"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, tt_SquirrelBind);
	sq_setclassudsize(v, -1, sizeof(WeakPtr<SquirrelShare>));
	register_closure(v, _SC("_get"), &SquirrelShare::sqf_get);
	register_closure(v, _SC("_set"), &SquirrelShare::sqf_set);
	sq_createslot(v, -3);
}





/// \brief List of registered initialization functions for ServerGame object.
///
/// Using construct on first use idiom to avoid static initialization order fiasco.
Game::ServerInitList &Game::serverInits(){
	static ServerInitList s_inits;
	return s_inits;
}

/// \brief Register initialization function to be executed when a ServerGame instance is created.
///
/// The caller must call this function at initialization step, typically the constructor of static class instance.
/// The constructor of ServerGame will automatically call those registered functions to perform initial operations
/// required for various classes, including extensions.
void Game::addServerInits(ServerInitFunc f){
	serverInits().push_back(f);
}

/// \brief List of registered initialization functions for ClientGame object.
///
/// Using construct on first use idiom to avoid static initialization order fiasco.
///
/// TODO: It will be compiled in dedicated server even though it will never be called.
Game::ClientInitList &Game::clientInits(){
	static ClientInitList s_inits;
	return s_inits;
}

/// \brief Register initialization function to be executed when a ClientGame instance is created.
///
/// Remarks are the same as addServerInits().
void Game::addClientInits(ClientInitFunc f){
	clientInits().push_back(f);
}

const Game::IdMap &Game::idmap()const{
	return idunmap;
}

Game::IdMap &Game::idmap(){
	return idunmap;
}

void Game::idUnmap(UnserializeContext &sc){
	unsigned long delsize;
	sc.i >> delsize;
	clientDeleting = true;
	for(int i = 0; i < delsize; i++){
		unsigned long id;
		sc.i >> id;
		IdMap::iterator it = idunmap.find(id);
		if(it != idunmap.end()){
			delete it->second;
			idunmap.erase(it);
			sc.syncbuf->erase(id);
		}
	}
	clientDeleting = false;

	// We'll reconstruct player list from the input stream, so we need to clear the existing one.
	players.clear();

	// Iterate through SyncBuf entries to apply patches to each of them.
	SyncBuf::iterator it0 = sc.syncbuf->begin();
	for(; it0 != sc.syncbuf->end(); it0++){
		UnserializeStream *us = new BinUnserializeStream(&it0->second.front(), it0->second.size(), &sc);
		cpplib::dstring src;
		unsigned long thisid = it0->first;
		*us >> src;
		UnserializeMap::iterator it = idunmap.find(thisid);
		if(it == idunmap.end()){

			// Try to match client-generated objects with server sent ones.
			ObjSet *objSet = getClientObjSet();
			if(objSet){
				double bestSDist = 1e6 * 1e6;
				Entity *bestObj = NULL;
				ObjSet::iterator bestIt = objSet->end();

				bool entityCache = false; // Availability of partially cached data for Entity.
				SerializableId warId; // Object ID for Entity's belonging WarField.
				Vec3d pos; // Entity's position.

				for(Game::ObjSet::iterator it = objSet->begin(); it != objSet->end(); ++it){
					Entity *o = *it;
					if(o->classname() == src){

						// We want to cache partially unserialized variables only in descendants of Entity,
						// otherwise we could end up interpreting variables having wrong types, or in the
						// worst case raise InputShortageException.
						// But we want to cache no more than once per SyncBuf object.
						// We cannot tell if a Serializable object is a descendant of Entity without
						// actually instantiating one, but if the classname matches to some client object,
						// it should be an Entity. In that case, we can cache the variables here.
						if(!entityCache){
							// We do not convert warId to WarField pointer here, because the WarField object
							// can be yet to be created in the client.
							*us >> warId;
							*us >> pos;
							entityCache = true;
						}

						// Try to match the belonging WarField.
						if(!o->w || warId != o->w->getid())
							continue;
						double sdist = (pos - o->pos).slen();
						if(sdist < bestSDist){
							bestSDist = sdist;
							bestObj = o;
							bestIt = it;
						}
					}
				}

				// Now that we matched this Entity, promote the client-generated Entity to server-generated one.
				if(bestObj){
					bestObj->id = thisid;
					idunmap[thisid] = bestObj;
					objSet->erase(bestIt); // Once we bind "phantom" objects to real ones, we can forget about them.
					continue;
				}
			}

			gltestp::dstring scname((const char*)src);
			{
				::CtorMap::iterator it = sc.cons.find(scname);
				if(it == sc.cons.end())
					throw ClassNotFoundException();
				if(it->second){
					Serializable *ret = it->second(this);
					ret->game = this;
					ret->id = thisid;
					idunmap[thisid] = ret;
					if(src == "Universe"){
						sq_replaceUniverse(static_cast<Universe*>(ret));
					}
					if(src == "Player"){
						players.push_back(static_cast<Player*>(ret));
					}
					if(src == "SquirrelShare"){
						setSquirrelShare(static_cast<SquirrelShare*>(ret));
					}
				}
			}
		}
		else{
			// Report anomaly as early as possible
			if(src != it->second->classname())
				throw ClassNotFoundException();
			if(src == "Player"){
				players.push_back(static_cast<Player*>(it->second));
			}
		}
		delete us;
	}

	// Delete client generated Entities because we could loose references to them
	// in the process of unserialization. We cannot reclaim memory occupied by the
	// object which had lost reference to it.
	if(Game::ObjSet *objSet = getClientObjSet()){
		for(Game::ObjSet::iterator it = objSet->begin(); it != objSet->end();){
			Entity *o = static_cast<Entity*>(*it);
			Game::ObjSet::iterator next = it;
			++next;
//				objSet->erase(it); // Since we delete all, no need to erase one by one, just single clear() is enough.
			it = next;
			delete o;
		}
		objSet->clear();
	}

}

void Game::idUnserialize(UnserializeContext &usc){
	unsigned long delsize;
	usc.i >> delsize;
	for(int i = 0; i < delsize; i++){
		unsigned long id;
		usc.i >> id;
	}

	assert(usc.syncbuf);
	unsigned l = 1;
	SyncBuf::iterator it0 = usc.syncbuf->begin();
	for(; it0 != usc.syncbuf->end(); it0++){
		SerializableId thisid = it0->first;
		cpplib::dstring name;
		UnserializeStream *us = new BinUnserializeStream(&it0->second.front(), it0->second.size(), &usc);
		UnserializeContext sc2(*us, usc.cons, usc.map);
		us->usc = &sc2;
		sc2.i >> name;
		IdMap::iterator it = usc.map.find(thisid);
		if(it != usc.map.end())
			it->second->unserialize(sc2);
		delete us;
		l++;
	}
}

void Game::serialize(SerializeStream &ss){
	Serializable* visit_list = NULL;

	// Update delete queue to notify that particular objects should be dead, destructed and freed.
	ss << (unsigned)deleteque.size();
	DeleteQue::iterator it = deleteque.begin();
	for(; it != deleteque.end(); it++){
		ss << *it;
		idmap().erase(*it); // Delete from the server game too.
	}
	deleteque.clear();

	// The second pass actually writes to the stream, replacing pointers with the object ids.
	SerializeContext sc(ss, visit_list);
	ss.sc = &sc;
	for(std::vector<Player*>::iterator it = players.begin(); it != players.end(); it++)
		(*it)->dive(sc, &Serializable::idPackSerialize);
	if(sqshare)
		sqshare->dive(sc, &Serializable::idPackSerialize);
	universe->dive(sc, &Serializable::idPackSerialize);
	(sc.visit_list)->clearVisitList();
}

void Game::unserialize(UnserializeContext &usc){
}


void ServerGame::anim(double dt){
#if 0
#define TRYBLOCK(a) {try{a;}catch(std::exception e){fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());}catch(...){fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);}}
#else
#define TRYBLOCK(a) (a);
#endif
		if(!universe->paused) try{
			for(int i = 0; i < players.size(); i++)
				players[i]->anim(dt);
			sqa_anim(sqvm, dt);
			TRYBLOCK(universe->anim(dt));
			TRYBLOCK(universe->endframe());
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);
		}

}

void Game::sq_replaceUniverse(Universe *p){
	universe = p;
	HSQUIRRELVM v = sqvm;
	sq_pushstring(v, _SC("universe"), -1); // this "universe"
	Universe::sq_pushobj(v, p);
	sq_createslot(v, 1); // this
}

void Game::sq_replacePlayer(Player *p){
	player = p;
	HSQUIRRELVM v = sqvm;
	sq_pushstring(v, _SC("player"), -1); // this "player"
	Player::sq_pushobj(v, p);
	sq_createslot(v, 1); // this
}

void Game::init(){
	sqa_anim0(sqvm);
}

/// Defaults true
bool Game::isRawCreateMode()const{
	return true;
}


void Game::setSquirrelShare(SquirrelShare *p){
	sqshare = p;
	HSQUIRRELVM v = sqvm;
	sq_pushstring(v, _SC("squirrelShare"), -1); // this "squirrelShare"
	SquirrelShare::sq_pushobj(v, p);
	sq_createslot(v, 1); // this
}





#ifndef DEDICATED
ClientGame::ClientGame() : loading(false){
	ClientInitList &inits = clientInits();
	for(int i = 0; i < inits.size(); i++)
		inits[i](*this);
}

/// If we create an object in a client, it's really an object, but
/// we can reconstruct objects from update stream sent from the server,
/// in which case objects are not really newly created.
bool ClientGame::isRawCreateMode()const{
	return loading;
}

/// Assume Object IDs in the upper half of the value range reserved for
/// the client side objects.
Serializable::Id ClientGame::nextId(){
	return idGenerator++ + UINT_MAX / 2;
}
#endif




ServerGame::ServerGame() : loading(false){
	player = new Player(this);
	player->playerId = 0;
	players.push_back(player);
	universe = new Universe(this);
	player->cs = universe;
	ServerInitList &inits = serverInits();
	for(int i = 0; i < inits.size(); i++)
		inits[i](*this);
}

void ServerGame::init(){
//	anim_sun(0.);
	Game::init();

	universe->anim(0.);
}

bool ServerGame::isRawCreateMode()const{
	return loading;
}
