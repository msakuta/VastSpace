/** \file 
 * \brief Implementation of Game class
 */

#include "antiglut.h"
#include "serial_util.h"
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include "Game.h"


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

std::map<unsigned long, Serializable *> idunmap;

const Game::IdMap &Game::idmap()const{
	return idunmap;
}

void Game::idUnmap(UnserializeContext &sc){
	std::vector<unsigned long> cldeleteque;
	unsigned long delsize;
	sc.i >> delsize;
	for(int i = 0; i < delsize; i++){
		unsigned long id;
		sc.i >> id;
		std::map<unsigned long, Serializable*>::iterator it = idunmap.find(id);
		if(it != idunmap.end()){
			delete it->second;
			idunmap.erase(it);
		}
	}

	// We'll reconstruct player list from the input stream, so we need to clear the existing one.
	players.clear();

	while(!sc.i.eof()){
		unsigned long size;
		sc.i >> size;
		if(sc.i.fail())
			break;
		UnserializeStream *us = sc.i.substream(size);
		cpplib::dstring src;
		unsigned long thisid;
		*us >> src;
		*us >> thisid;
		std::map<unsigned long, Serializable *>::iterator it = idunmap.find(thisid);
		if(it == idunmap.end()){
			gltestp::dstring scname((const char*)src);
			/*if(src != "Player" && src != "Universe")*/{
				::CtorMap::iterator it = sc.cons.find(scname);
				if(it == sc.cons.end())
					throw ClassNotFoundException();
				if(it->second){
					Serializable *ret = it->second();
					ret->id = thisid;
					sc.map.push_back(ret);
					idunmap[thisid] = ret;
					if(src == "Universe"){
						universe = static_cast<Universe*>(ret);
					}
					if(src == "Player"){
						players.push_back(static_cast<Player*>(ret));
					}
				}
			}
		}
		else{
			// Report anomaly as early as possible
			if(src != it->second->classname())
				throw ClassNotFoundException();
			sc.map.push_back(it->second);
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
	unsigned l = 1;
	while(/*!usc.i.eof() &&*/ l < usc.map.size()){
		usc.map[l]->idPackUnserialize(usc);
		l++;
	}
}

void Game::serialize(SerializeStream &ss){
	extern std::vector<unsigned long> deleteque;
	SerializeMap map;
	map[NULL] = 0;
//	map[player] = 1;
	Serializable* visit_list = NULL;
	SerializeContext sc0(*(SerializeStream*)NULL, map, visit_list);
	for(std::vector<Player*>::iterator it = players.begin(); it != players.end(); it++)
		(*it)->dive(sc0, &Serializable::map);
	universe->dive(sc0, &Serializable::map);

	ss << (unsigned long)deleteque.size();
	std::vector<unsigned long>::iterator it = deleteque.begin();
	for(; it != deleteque.end(); it++)
		ss << *it;
	deleteque.clear();

	SerializeContext sc(ss, map, visit_list);
	ss.sc = &sc;
	for(std::vector<Player*>::iterator it = players.begin(); it != players.end(); it++)
		(*it)->dive(sc, &Serializable::idPackSerialize);
	universe->dive(sc, &Serializable::idPackSerialize);
	(sc.visit_list)->clearVisitList();
}

void Game::unserialize(UnserializeContext &usc){
}


void Game::anim(double dt){
	universe->anim(dt);
	universe->postframe();
}









ServerGame::ServerGame(){
	player = new Player();
	player->playerId = 0;
	players.push_back(player);
	universe = new Universe(player);
	player->cs = universe;
	for(int i = 0; i < nserverInits; i++)
		serverInits[i](*this);
}
