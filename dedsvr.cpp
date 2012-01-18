#include "Application.h"
#include "Game.h"
#include <stdio.h>

DedicatedServerApplication application;

int main(int argc, char *argv[]){
	printf("Hello Universe %p\n", &application);
}

//-----------------------------------------------------------------------------
//  ClientMessage implementation
//-----------------------------------------------------------------------------

ClientMessage::CtorMap &ClientMessage::ctormap(){
	static CtorMap s;
	return s;
}

ClientMessage::ClientMessage(dstring id){
	this->id = id;
	ctormap()[id] = this;
}

ClientMessage::~ClientMessage(){
	ctormap().erase(id);
}

void ClientMessage::send(Application &cl, const void *p, size_t size){
}
