#include "Universe.h"
#include "serial_util.h"
#include "Player.h"
#include "cmd.h"
#include "astro.h"
#include "sqadapt.h"
extern "C"{
#include "calc/calc.h"
#include <clib/mathdef.h>
}
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>

#define ENABLE_TEXTFORMAT 1

// Increment whenever serialization specification changes in any Serializable object.
const unsigned Universe::version = 10;

ClassRegister<Universe> Universe::classRegister("Universe", sq_define);

Universe::Universe(Player *pl, Game *game) : ppl(pl), game(game), paused(true), timescale(1), global_time(0), astro_time(0),
	gravityfactor(1.)
{
	name = new char[sizeof"root"];
	strcpy(const_cast<char*>(name), "root");
	fullname = NULL;
	flags = CS_ISOLATED | CS_EXTENT;
}

Universe::~Universe(){
}

void Universe::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << timescale << global_time;
	sc.o << astro_time;
	sc.o << paused;
}

extern std::istream &operator>>(std::istream &o, const char *cstr);

void Universe::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> timescale >> global_time;
	sc.i >> astro_time;
	sc.i >> paused;
}

void Universe::anim(double dt){
	this->global_time += dt;
	this->astro_time += Astrobj::astro_timescale * dt;
	st::anim(dt);
}

void Universe::csUnmap(UnserializeContext &sc){
	while(!sc.i.eof()){
		unsigned long size;
		sc.i >> size;
		if(sc.i.fail())
			break;
		UnserializeStream *us = sc.i.substream(size);
		cpplib::dstring src;
		*us >> src;
		gltestp::dstring scname((const char*)src);
		if(src != "Player" && src != "Universe"){
			::CtorMap::iterator it = sc.cons.find(scname);
			if(it == sc.cons.end())
				throw ClassNotFoundException();
			if(it->second){
				sc.map.push_back(it->second());
			}
		}
		delete us;
	}
}

void Universe::csUnserialize(UnserializeContext &usc){
	unsigned l = 1;
	while(/*!usc.i.eof() &&*/ l < usc.map.size()){
		usc.map[l]->packUnserialize(usc);
		l++;
	}
}



int Universe::cmd_save(int argc, char *argv[], void *pv){
	Universe &universe = *(Universe*)pv;
	Player &pl = *universe.ppl;
	SerializeMap map;
	bool text = argc < 3 ? false : !strcmp(argv[2], "t");
	const char *fname = argc < 2 ? text ? "savet.sav" : "saveb.sav" : argv[1];
	map[NULL] = 0;
	map[&pl] = map.size();
	Serializable* visit_list = NULL;
	{
		SerializeContext sc(*(SerializeStream*)NULL, map, visit_list);
		universe.dive(sc, &Serializable::map);
	}
#if ENABLE_TEXTFORMAT
	if(text){
		std::fstream fs(fname, std::ios::out | std::ios::binary);
		fs << "savetext";
		fs << version;
		StdSerializeStream sss(fs);
		SerializeContext sc(sss, map, visit_list);
		sss.sc = &sc;
		pl.packSerialize(sc);
		universe.dive(sc, &Serializable::packSerialize);
		(sc.visit_list)->clearVisitList();
	}
	else
#endif
	{
		BinSerializeStream bss;
		SerializeContext sc(bss, map, visit_list);
		bss.sc = &sc;
		pl.packSerialize(sc);
		universe.dive(sc, &Serializable::packSerialize);
		(sc.visit_list)->clearVisitList();
		FILE *fp = fopen(fname, "wb");
		fputs("savebina", fp);
		fwrite(&version, sizeof version, 1, fp);
		fwrite(bss.getbuf(), 1, bss.getsize(), fp);
		fclose(fp);
	}
	return 0;
}

extern int cs_destructs;
int Universe::cmd_load(int argc, char *argv[], void *pv){
	const char *fname = argc < 2 ? "saveb.sav" : argv[1];
#ifdef _WIN32
	if(-1 == GetFileAttributes(fname)){
		CmdPrint("Specified file does not exist.");
		return 0;
	}
#else
	{
		FILE *fp = fopen(fname, "r");
		if(fp)
			fclose(fp);
		else{
			CmdPrintf("Specified file does not exist.");
			return 0;
		}
	}
#endif
	char signature[8];

	// Check file consistency before deleting existing universe.
	{
		FILE *fp = fopen(fname, "rb");
		if(!fp){
			CmdPrintf("File %s could not be opened.", fname);
			return 0;
		}
		if(0 == fread(signature, sizeof signature, 1, fp)){
			CmdPrintf("File %s is broken or unknown format.", fname);
			fclose(fp);
			return 0;
		}
		if(!memcmp(signature, "savebina", sizeof signature)){
			unsigned fileversion;
			if(!fread(&fileversion, sizeof fileversion, 1, fp) || fileversion != version){
				CmdPrintf("Saved file version number %d is different as %d.", fileversion, version);
				fclose(fp);
				return 0;
			}
		}
#if ENABLE_TEXTFORMAT
		else if(!memcmp(signature, "savetext", sizeof signature)){
			unsigned fileversion;
			if(!fscanf(fp, "%u", &fileversion) || fileversion != version){
				CmdPrintf("Saved file version number %d is different as %d.", fileversion, version);
				fclose(fp);
				return 0;
			}
		}
#endif
		else{
			CmdPrintf("");
			fclose(fp);
			return 0;
		}
		fclose(fp);
	}

	Universe &universe = *(Universe*)pv;
	Player &pl = *universe.ppl;
	cpplib::dstring plpath = pl.cs->getpath();
	cs_destructs = 0;

	for(CoordSys *cs = universe.children; cs;){
		CoordSys *csnext = cs->next;
		delete cs;
		cs = csnext;
	}

//	delete universe.children;
//	delete universe.next;
	pl.free();
	printf("destructs %d\n", cs_destructs);
	universe.aorder.clear();
	std::vector<Serializable*> map;
	map.push_back(NULL);
	map.push_back(&pl);
	map.push_back(&universe);
	unsigned char *buf;
	long size;
	do{
		{
			FILE *fp = fopen(fname, "rb");
			if(!fp)
				return 0;
			fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fread(signature, 1, sizeof signature, fp);
			size -= sizeof signature;
			buf = new unsigned char[size];
			fread(buf, 1, size, fp);
			fclose(fp);
		}
		if(!memcmp(signature, "savebina", sizeof signature)){
			// Very special treatment of version number.
			unsigned fileversion = *(unsigned*)buf;
			if(fileversion != version){
				CmdPrintf("Saved file version number %d is different as %d.", fileversion, version);
				break;
			}
			{
				BinUnserializeStream bus(&buf[sizeof fileversion], size - sizeof fileversion);
				UnserializeContext usc(bus, Serializable::ctormap(), map);
				bus.usc = &usc;
				universe.csUnmap(usc);
			}
			{
				BinUnserializeStream bus(&buf[sizeof fileversion], size - sizeof fileversion);
				UnserializeContext usc(bus, Serializable::ctormap(), map);
				bus.usc = &usc;
				universe.csUnserialize(usc);
			}
		}
	#if ENABLE_TEXTFORMAT
		else if(!memcmp(signature, "savetext", sizeof signature)){
			// Very special treatment of version number.
			char *end;
			long fileversion = ::strtol((char*)buf, &end, 10);
			if(fileversion != version){
				CmdPrintf("Saved file version number %d is different as %d.", fileversion, version);
				break;
			}
			size -= end - (char*)buf;
			{
				std::istringstream ss(std::string(end, size));
				StdUnserializeStream sus(ss);
				UnserializeContext usc(sus, Serializable::ctormap(), map);
				sus.usc = &usc;
				universe.csUnmap(usc);
			}
			{
				std::stringstream ss(std::string(end, size));
				StdUnserializeStream sus(ss);
				UnserializeContext usc(sus, Serializable::ctormap(), map);
				sus.usc = &usc;
				universe.csUnserialize(usc);
			}
		}
	#endif
		else{
			CmdPrint("Unrecognized save file format");
		}
	}while(0);
	delete[] buf;
	return 0;
}

SQInteger Universe::sqf_get(HSQUIRRELVM v){
	Universe *p;
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("timescale"))){
		sq_pushfloat(v, SQFloat(p->timescale));
		return 1;
	}
	else if(!strcmp(wcs, _SC("global_time"))){
		sq_pushfloat(v, SQFloat(p->global_time));
		return 1;
	}
	else
		return st::sqf_get(v);
}

bool Universe::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	register_closure(v, _SC("_get"), sqf_get);
	sq_createslot(v, -3);
	return true;
}

#ifndef _WIN32
void Universe::draw(const Viewer*){}
#endif
