/** \file
 * \brief Implementation of SqInitProcess class and its companions.
 */
#include "SqInitProcess.h"
#include "sqadapt.h"
#include "cmd.h"
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

