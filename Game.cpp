/** \file 
 * \brief Implementation of Game class
 */

#include "antiglut.h"
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

void Game::serialize(SerializeContext &sc){
}

void Game::unserialize(UnserializeContext &usc){
}
