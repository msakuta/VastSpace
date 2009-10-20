#include "astro.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "astrodef.h"
extern "C"{
#include "calc/calc.h"
}
#include <string.h>
#include <stdlib.h>

Astrobj **astrobjs;
int nastrobjs;

Astrobj::Astrobj(const char *name, CoordSys *cs) : CoordSys(name, cs){
	CoordSys *eis = findeisystem();
	if(eis){
		eis->aorder = (CoordSys**)realloc(eis->aorder, (eis->naorder + 1) * sizeof *eis->aorder);
		eis->aorder[eis->naorder++] = this;
	}
}

bool Astrobj::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "mass")){
		if(argv[1]){
			mass = calc3(const_cast<const char**>(&argv[1]), sc.vl, NULL);
			if(flags & AO_BLACKHOLE)
				rad = 2 * UGC * mass / LIGHT_SPEED / LIGHT_SPEED;
		}
		return true;
	}
	else if(!strcmp(s, "gravity")){
		if(1 < argc){
			if(atoi(argv[1]))
				flags |= AO_GRAVITY;
			else
				flags &= ~AO_GRAVITY;
		}
		else
			flags |= AO_GRAVITY;
	}
	else if(!strcmp(s, "spherehit")){
		if(1 < argc){
			if(atoi(argv[1]))
				flags |= AO_SPHEREHIT;
			else
				flags &= ~AO_SPHEREHIT;
		}
		else
			flags |= AO_SPHEREHIT;
	}
	else if(!strcmp(s, "planet")){
		if(1 < argc){
			if(atoi(argv[1]))
				flags |= AO_PLANET;
			else
				flags &= ~AO_PLANET;
		}
		else
			flags |= AO_PLANET;
	}
	else
		return CoordSys::readFile(sc, argc, argv);
	return true;
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

Star::Star(const char *name, CoordSys *cs) : Astrobj(name, cs){}

const char *Star::classname()const{
	return "Star";
}
