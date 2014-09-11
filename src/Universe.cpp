#include "Universe.h"
#include "serial_util.h"
#include "Player.h"
#include "cmd.h"
#include "astro.h"
#include "sqadapt.h"
#include "Application.h"
#include "ClientMessage.h"
#include "Game.h"
extern "C"{
#include <clib/mathdef.h>
}
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>

#define ENABLE_TEXTFORMAT 1

// Increment whenever serialization specification changes in any Serializable object.
const unsigned Universe::version = 12;

ClassRegister<Universe> Universe::classRegister("Universe", sq_define);

Universe::Universe(Game *game) : st(game), paused(true), timescale(1), global_time(0), astro_time(0),
	gravityfactor(1.), astro_timescale(1.)
{

	if(game->isServer()){
		name = "root";
		fullname = "";
		flags = CS_ISOLATED | CS_EXTENT;
	}
}

Universe::~Universe(){
}

void Universe::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << timescale << global_time;
	sc.o << astro_time;
	sc.o << paused;
	sc.o << gravityfactor;
	sc.o << astro_timescale;
}

extern std::istream &operator>>(std::istream &o, const char *cstr);

void Universe::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> timescale >> global_time;
	sc.i >> astro_time;
	sc.i >> paused;
	sc.i >> gravityfactor;
	sc.i >> astro_timescale;
}

void Universe::anim(double dt){
	this->global_time += dt;
	this->astro_time += astro_timescale * dt;
	st::anim(dt);
}

void Universe::clientUpdate(double dt){
	// Delegate to server update failed because of st::anim(dt).
	this->global_time += dt;
	this->astro_time += astro_timescale * dt;
	st::clientUpdate(dt);
}

void Universe::csUnmap(UnserializeContext &sc){
#if 1 // Disabled because you cannot know which Entity to unserialize this buffer without parsing embedded SerializableId.
	assert(0);
#else
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
				sc.map[it->first] = (it->second());
			}
		}
		delete us;
	}
#endif
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
	Player &pl = *universe.game->player;
	SerializeMap map;
	bool text = argc < 3 ? false : !strcmp(argv[2], "t");
	const char *fname = argc < 2 ? text ? "savet.sav" : "saveb.sav" : argv[1];
	map[NULL] = 0;
	map[&pl] = map.size();
	Serializable* visit_list = NULL;
	{
		SerializeContext sc(*(SerializeStream*)NULL, visit_list);
		universe.dive(sc, &Serializable::map);
	}
#if ENABLE_TEXTFORMAT
	if(text){
		std::fstream fs(fname, std::ios::out | std::ios::binary);
		fs << "savetext";
		fs << version;
		StdSerializeStream sss(fs);
		SerializeContext sc(sss, visit_list);
		sss.sc = &sc;
		pl.packSerialize(sc);
		universe.dive(sc, &Serializable::packSerialize);
		(sc.visit_list)->clearVisitList();
	}
	else
#endif
	{
		BinSerializeStream bss;
		SerializeContext sc(bss, visit_list);
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
	Player &pl = *universe.game->player;
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
	UnserializeMap map;
	map[0] = (NULL);
//	map.push_back(&pl);
//	map.push_back(&universe);
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
	Universe *p = sq_refobj(v)->toUniverse();
	const SQChar *wcs;
	sq_getstring(v, -1, &wcs);
	if(!strcmp(wcs, _SC("paused"))){
		sq_pushbool(v, SQBool(p->paused));
		return 1;
	}
	else if(!strcmp(wcs, _SC("timescale"))){
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

struct CMPause : public ClientMessage{
	typedef ClientMessage st;
	static CMPause s;
	void interpret(ServerClient &sc, UnserializeStream &uss);
	static void send(bool);
private:
	CMPause();
};

CMPause CMPause::s;

CMPause::CMPause() : st("Pause"){}

void CMPause::send(bool b){
	std::stringstream ss;
	StdSerializeStream sss(ss);
	sss << b;
	std::string str = ss.str();
	s.st::send(application, str.c_str(), str.size());
}

void CMPause::interpret(ServerClient &sc, UnserializeStream &uss){
	bool b;
	uss >> b;
	if(sc.sv->pg->universe)
		sc.sv->pg->universe->paused = b;
}

/*int cmd_pause(int argc, char *argv[], void *pv){
	Game *game = (Game*)pv;
	if(argc <= 1){
		CmdPrint(cpplib::dstring() << "pause is " << (game->universe ? game->universe->paused : false));
		return 0;
	}
	CMPause::send(atoi(argv[1]));
	return 0;
}*/

SQInteger Universe::sqf_set(HSQUIRRELVM v){
	Universe *p = sq_refobj(v)->toUniverse();
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!strcmp(wcs, _SC("paused"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return sq_throwerror(v, _SC("paused member must be bool compatible"));
		p->paused = !!b;
		if(!p->game->isServer() || !p->game->isClient())
			CMPause::s.send(!!b);
		return 0;
	}
	else
		return SQ_ERROR;
	// TODO: Not available yet!
//		return st::sqf_set(v);
}

bool Universe::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, classRegister.s_sqclassname, -1);
	sq_pushstring(v, st::classRegister.s_sqclassname, -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, SQUserPointer(classRegister.id));
	sq_setclassudsize(v, -1, sq_udsize); // classudsize is not inherited from CoordSys
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("_set"), sqf_set);
	sq_createslot(v, -3);
	return true;
}

#ifndef _WIN32
void Universe::draw(const Viewer*){}
#endif
