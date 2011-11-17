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

void Game::serialize(SerializeStream &ss){
	extern std::vector<unsigned long> deleteque;
	SerializeMap map;
	map[NULL] = 0;
	map[player] = 1;
	Serializable* visit_list = NULL;
	SerializeContext sc0(*(SerializeStream*)NULL, map, visit_list);
	universe->dive(sc0, &Serializable::map);

	ss << (unsigned long)deleteque.size();
	std::vector<unsigned long>::iterator it = deleteque.begin();
	for(; it != deleteque.end(); it++)
		ss << *it;
	deleteque.clear();

	SerializeContext sc(ss, map, visit_list);
	ss.sc = &sc;
	player->idPackSerialize(sc);
	universe->dive(sc, &Serializable::idPackSerialize);
	(sc.visit_list)->clearVisitList();
}

void Game::unserialize(UnserializeContext &usc){
}


void Game::anim(double dt){
	universe->anim(dt);
	universe->postframe();
}
