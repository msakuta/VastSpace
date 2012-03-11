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





/// List of registered initialization functions. Too bad std::vector cannot be used because the class static parameters
/// are not initialized.
void (*Game::serverInits[64])(Game&);
int Game::nserverInits = 0;

/// \brief Register initialization function to be executed when a ServerGame instance is created.
///
/// The caller must call this function at initialization step, typically the constructor of static class instance.
/// The constructor of ServerGame will automatically call those registered functions to perform initial operations
/// required for various classes, including extensions.
void Game::addServerInits(void (*f)(Game &)){
	serverInits[nserverInits++] = f;
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
			gltestp::dstring scname((const char*)src);
			{
				::CtorMap::iterator it = sc.cons.find(scname);
				if(it == sc.cons.end())
					throw ClassNotFoundException();
				if(it->second){
					Serializable *ret = it->second();
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
			sqa_anim(sqvm, dt);
			TRYBLOCK(universe->anim(dt));
			postframe();
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

bool Game::isServer()const{
	return false;
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
ClientGame::ClientGame(){}
#endif




ServerGame::ServerGame() : loading(false){
	player = new Player(this);
	player->playerId = 0;
	players.push_back(player);
	universe = new Universe(player, this);
	player->cs = universe;
	for(int i = 0; i < nserverInits; i++)
		serverInits[i](*this);
}

void ServerGame::init(){
//	anim_sun(0.);
	universe->anim(0.);

	Game::init();
}

void ServerGame::postframe(){
	universe->postframe();
}

bool ServerGame::isServer()const{
	return true;
}

bool ServerGame::isRawCreateMode()const{
	return loading;
}
