/** \file
 * \brief Implements Entity class and its collaborative classes.
 */
#include "Entity.h"
#include "Application.h"
#include "EntityCommand.h"
extern "C"{
#include <clib/aquat.h>
}
#include "Beamer.h"
#include "judge.h"
#include "Observable.h"


bool Observer::unlink(Observable *){
	// Do nothing and allow unlinking by default
	return true;
}

Observable::~Observable(){
	for(ObserverList::iterator it = observers.begin(); it != observers.end(); it++){
		it->first->unlink(this);
	}
	for(WeakPtrList::iterator it = ptrs.begin(); it != ptrs.end(); it++){
		(*it)->unlink(this);
	}
}

void Observable::addObserver(Observer *o){
	ObserverList::iterator it = observers.find(o);
	if(it != observers.end())
		it->second++;
	else
		observers[o] = 1;
}

void Observable::removeObserver(Observer *o){
	ObserverList::iterator it = observers.find(o);
	assert(it != observers.end());
	if(0 == --it->second)
		observers.erase(it);
}

void Observable::addWeakPtr(WeakPtrBase *o){
	WeakPtrList::iterator it = ptrs.find(o);
	if(it == ptrs.end())
		ptrs.insert(o);
}

void Observable::removeWeakPtr(WeakPtrBase *o){
	WeakPtrList::iterator it = ptrs.find(o);
	assert(it != ptrs.end());
	ptrs.erase(it);
}
