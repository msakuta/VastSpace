#include "astro.h"
#include "stellar_file.h"
#include <string.h>

Astrobj **astrobjs;
int nastrobjs;

Astrobj::Astrobj(const char *name, CoordSys *cs){
	init(name, cs);
}

bool Astrobj::readFile(StellarContext &sc, int argc, char *argv[]){
	return CoordSys::readFile(sc, argc, argv);
}

const char *Astrobj::classname()const{
	return "Astrobj";
}

Astrobj *findastrobj(const char *name){
	int i;
	if(!name)
		return NULL;
	for(i = nastrobjs-1; 0 <= i; i--) if(!strcmp(astrobjs[i]->name, name)){
		return astrobjs[i];
	}
	return NULL;
}

