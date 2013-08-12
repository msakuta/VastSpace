/** \file
 * \brief Implementation of SqInitProcess class and its companions.
 */
#include "SqInitProcess-ex.h"
#include "sqadapt.h"
#include "cmd.h"
#include "arms.h"
extern "C"{
#include <clib/timemeas.h>
}


/// \brief The function that is called to initialize static customizable variables to a specific Entity class.
///
/// Be aware that this function can be called in both the client game and the server game.
/// It depends on initialization order. Specifically, dedicated server always invoke in the server game,
/// while the Windows client invokes in the client game if it's connected to a server.
/// As for a standalone server, it's not determined.
bool SqInitProcess::SqInit(HSQUIRRELVM v, const SQChar *scriptFile, const SqInitProcess &procs){
	try{
		StackReserver sr(v);
		timemeas_t tm;
		TimeMeasStart(&tm);
		sq_newtable(v);
		sq_pushroottable(v); // root
		sq_setdelegate(v, -2);
		if(SQ_SUCCEEDED(sqa_dofile(v, scriptFile, 0, 1))){
			const SqInitProcess *proc = &procs;
			for(; proc; proc = proc->next)
				proc->process(v);
		}
		double d = TimeMeasLap(&tm);
		CmdPrint(gltestp::dstring() << scriptFile << " total: " << d << " sec");
	}
	catch(SQFError &e){
		// TODO: We wanted to know which line caused the error, but it seems it's not possible because the errors
		// occur in the deferred loading callback objects.
/*		SQStackInfos si0;
		if(SQ_SUCCEEDED(sq_stackinfos(game->sqvm, 0, &si0))){
			SQStackInfos si;
			for(int i = 1; SQ_SUCCEEDED(sq_stackinfos(game->sqvm, i, &si)); i++)
				si0 = si;
			CmdPrint(gltestp::dstring() << scriptFile << "(" << int(si0.line) << "): " << si0.funcname << ": error: " << e.what());
		}
		else*/
			CmdPrint(gltestp::dstring() << scriptFile << " error: " << e.what());
	}
	return true;
}

void IntProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string
	if(SQ_FAILED(sq_get(v, -2))){
		if(mandatory) // If mandatory, read errors result in exceptions.
			throw SQFError(gltestp::dstring(name) << _SC(" not found"));
		else // If not mandatory variable cannot be read, leave the default value and silently ignore.
			return;
	}
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, -1, &i))){
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" couldn't be converted to float"));
		else
			return;
	}
	value = int(i);
	sq_poptop(v);
}

void SingleDoubleProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string
	if(SQ_FAILED(sq_get(v, -2))){
		if(mandatory) // If mandatory, read errors result in exceptions.
			throw SQFError(gltestp::dstring(name) << _SC(" not found"));
		else // If not mandatory variable cannot be read, leave the default value and silently ignore.
			return;
	}
	SQFloat f;
	if(SQ_FAILED(sq_getfloat(v, -1, &f))){
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" couldn't be converted to float"));
		else
			return;
	}
	value = double(f);
	sq_poptop(v);
}

void StringProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string
	if(SQ_FAILED(sq_get(v, -2))){
		if(mandatory) // If mandatory, read errors result in exceptions.
			throw SQFError(gltestp::dstring(name) << _SC(" not found"));
		else // If not mandatory variable cannot be read, leave the default value and silently ignore.
			return;
	}
	const SQChar *f;
	if(SQ_FAILED(sq_getstring(v, -1, &f))){
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" couldn't be converted to string"));
		else
			return;
	}
	value = f;
	sq_poptop(v);
}

namespace sqinitproc{

void StringListProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string

	if(SQ_FAILED(sq_get(v, -2))){ // root obj
		// Report error only if desired values are necessary and lacking.
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" not defined"));
		else
			return;
	}
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(gltestp::dstring(name) << _SC(" size could not be acquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		const SQChar *r;
		sq_getstring(v, -1, &r);
		value.push_back(r);
		sq_poptop(v); // root obj obj[i]
	}
	sq_poptop(v); // root
}

void Vec3dListProcess::process(HSQUIRRELVM v)const{
	sq_pushstring(v, name, -1); // root string

	// Not defining hardpoints is valid. Just ignore the case.
	if(SQ_FAILED(sq_get(v, -2))){ // root obj
		if(mandatory)
			throw SQFError(gltestp::dstring(name) << _SC(" not defined"));
		else
			return;
	}
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(gltestp::dstring(name) << _SC(" size could not be acquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		SQVec3d r;
		r.getValue(v, -1);
		value.push_back(r.value);
		sq_poptop(v); // root obj obj[i]
	}
	sq_poptop(v); // root
}

void HardPointProcess::process(HSQUIRRELVM v)const{
	Game *game = (Game*)sq_getforeignptr(v);

	// If it's the server, loading the settings from the local file is meaningless,
	// because they will be sent from the server.
	// Loading them in client does not hurt more than some little memory leaks,
	// but we clearly state here that the local settings won't ever be used.
	// UPDATE: We allow the client to initialize the hardpoint data, since we
	// allowed it to create an Entity before the Server tells about hardpoints.
	// If the settings do not match between the server and the client, it should
	// be forced to follow the server through unserialization.
/*	if(!game->isServer())
		return;*/

	sq_pushstring(v, _SC("hardpoints"), -1); // root string

	// Not defining hardpoints is valid. Just ignore the case.
	if(SQ_FAILED(sq_get(v, -2))) // root obj
		return;
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(_SC("hardpoints size could not be aquired"));
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		hardpoint_static *np = new hardpoint_static(game);
		hardpoint_static &n(*np);

		sq_pushstring(v, _SC("pos"), -1); // root obj obj[i] "pos"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].pos
			SQVec3d r;
			r.getValue(v, -1);
			n.pos = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else // Don't take it serously when the item is undefined, just assign the default.
			n.pos = Vec3d(0,0,0);

		sq_pushstring(v, _SC("rot"), -1); // root obj obj[i] "rot"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].rot
			SQQuatd r;
			r.getValue(v);
			n.rot = r.value;
			sq_poptop(v); // root obj obj[i]
		}
		else // Don't take it serously when the item is undefined, just assign the default.
			n.rot = Quatd(0,0,0,1);

		sq_pushstring(v, _SC("name"), -1); // root obj obj[i] "name"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].name
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr))){
				char *name = new char[strlen(sqstr) + 1];
				strcpy(name, sqstr);
				n.name = name;
			}
			else // Throw an error because there's no such thing like default name.
				throw SQFError("HardPointsProcess: name is not specified");
			sq_poptop(v); // root obj obj[i]
		}
		else // Throw an error because there's no such thing like default name.
			throw SQFError("HardPointsProcess: name is not specified");

		sq_pushstring(v, _SC("flagmask"), -1); // root obj obj[i] "flagmask"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].flagmask
			SQInteger i;
			if(SQ_SUCCEEDED(sq_getinteger(v, -1, &i)))
				n.flagmask = (unsigned)i;
			else
				n.flagmask = 0;
			sq_poptop(v); // root obj obj[i]
		}
		else
			n.flagmask = 0;

		hardpoints.push_back(np);
		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

}
