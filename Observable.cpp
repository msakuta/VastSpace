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

bool Observer::handleEvent(Observable *o, ObserveEvent &e){return false;}

Observable::~Observable(){
	for(ObserverList::iterator it = observers.begin(); it != observers.end(); it++){
		it->first->unlink(this);
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

/// Obsolete; should call addObserver from the first place.
void Observable::addWeakPtr(WeakPtrBase *o){
	addObserver(o);
}

/// Obsolete; should call removeObserver from the first place.
void Observable::removeWeakPtr(WeakPtrBase *o){
	removeObserver(o);
}

/// \brief Notifies the event to all observers that observe this object.
///
/// The event is virtual, so any event can be passed.
void Observable::notifyEvent(ObserveEvent &e){
	ObserverList::iterator it = observers.begin();
	for(; it != observers.end();){
		// The iterator pointed object can be cleared in it->handleEvent, so we
		// first reserve iterator to the next and then call handleEvent.
		ObserverList::iterator next = it;
		next++;
		it->first->handleEvent(this, e);
		it = next;
	}
}



//-----------------------------------------------------------------------------
//  ObserveEvent implementation
//-----------------------------------------------------------------------------

ObserveEvent::MapType &ObserveEvent::ctormap(){
	static MapType s;
	return s;
}

bool ObserveEvent::derived(ObserveEventID)const{return false;}

