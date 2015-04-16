#define _CRT_SECURE_NO_WARNINGS

#include "stellar_file.h"
//#include "RoundAstrobj.h"
#include "serial_util.h"
#include "argtok.h"
#include "sqadapt.h"
#include "Universe.h"
#include "Game.h"
//#include "cmd.h"
//#include "spacewar.h"
#include "astrodef.h"
#include "astro_star.h"
#include "Entity.h"
//#include "island3.h"
//#include "ringworld.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/timemeas.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <fstream>
#include <sstream>
#include <deque>

extern double gravityfactor;

typedef std::deque<gltestp::dstring> TokenList;
typedef long linenum_t; ///< I think 2 billions would be enough.

/// \brief The scanner class to interpret a Stellar Structure Definition file.
///
/// Can handle line comments, block comments and curly braces.
/// It would be even possible to suspend scanning per-character basis.
class StellarStructureScanner{
public:
	StellarStructureScanner(std::istream *fp, linenum_t line = 0);
	gltestp::dstring nextLine(TokenList* = NULL);
	bool eof()const{return 0 != fp->eof();}
	linenum_t getLine()const{return line;} /// Returns line number.
protected:
	std::istream *fp;
	linenum_t line;
	enum{Normal, LineComment, BlockComment, Quotes, Braces} state;
	gltestp::dstring buf;
	gltestp::dstring currentToken;
	TokenList tokens;
};

StellarStructureScanner::StellarStructureScanner(std::istream *fp, linenum_t line) : fp(fp), line(line), state(Normal){
}

/// Scan and interpret input stream, separate them into tokens, returns on end of line.
gltestp::dstring StellarStructureScanner::nextLine(TokenList *argv){
	buf = "";
	tokens.clear();
	currentToken = "";
	int c, lc = EOF;
	bool escaped = false; // We never yield scanning in escaped state, so it don't need to be a member of StellarStructureScanner.
	int braceNests = 0; // We never yield in the middle of braces.
	while((c = fp->get()) != EOF){

		// Return the line unless it's escaped by a backslash.
		if(c == '\n'){
			// Increment the line number regardless of whether escaped or not, because the line number is used by
			// the users who use ordinary editors to examine contents of Stellar Structure Definition files.
			line++;

			// Substitute the newline with a whitespace if escaped.
			if(escaped)
				c = ' ';
			else if(state == Braces)
				; // Do nothing
			else{
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
				if(argv){
					*argv = tokens;
				}
				if(state == LineComment)
					state = Normal;
				return buf;
			}
		}

		// If we have a backslash without escaping by backslash itself, skip it.
		if(!escaped && c == '\\'){
			escaped = true;
			continue;
		}

		switch(state){
		case Normal:

			// Line comment scan till newline
			if(c == '#'){
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
				state = LineComment;
				continue;
			}

			if(c == '*' && lc == '/'){
				if(1 < currentToken.len()){
					tokens.push_back(currentToken[1]);
				}
				currentToken = "";
				state = BlockComment;
				continue;
			}

			if(c == '{'){
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
				braceNests = 1;
				state = Braces;
				continue;
			}

			if(c == '"'){
				state = Quotes;
			}
			else if(isspace(c)){
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
			}
			else
				currentToken << char(c);

//			if(argv && isspace(c))
//				argv->push_back(buf);

			buf << char(c);
			break;

		case LineComment:
			if(c == '\n')
				state = Normal;
			break;

		case BlockComment:
			if(c == '/' && lc == '*')
				state = Normal;
			break;

		case Quotes:
			if(lc != '\\' && c == '"'){
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
				state = Normal;
			}
			else
				currentToken << char(c);

			buf << char(c);
			break;

		case Braces:
			// Read the whole string (including newlines!) until matching closing brace is found.
			if(c == '}' && --braceNests <= 0){
				tokens.push_back(currentToken);
				currentToken = "";
				state = Normal;
			}
			else{
				// Store the character into string verbatim
				currentToken << char(c);
				buf << char(c);

				// Count up braces to properly parse nested structure.
				if(c == '{')
					braceNests++;
			}
			break;
		}
		lc = c;
		escaped = false;
	}
				if(argv)
					*argv = tokens;
	return buf;
}









#define MAX_LINE_LENGTH 512

/* stack state machine */
static int StellarFileLoadInt(const char *fname, CoordSys *root, struct varlist *vl);
static int stellar_astro(StellarContext *sc, Astrobj *a);

int Game::stellar_coordsys(StellarContext &sc, CoordSys *cs){
	typedef CoordSys *(Game::*CC)(const char *name, CoordSys *cs);

	const char *fname = sc.fname;
	const CoordSys *root = sc.root;
	int mode = 1;
	int enable = 0;
	HSQUIRRELVM v = sqvm;
	cs->readFileStart(sc);
	sqa::StackReserver sr(v);

	// Create a temporary table for Squirrel to save CoordSys-local defines.
	// Defined variables in parent CoordSys's can be referenced by delegation.
	sq_newtable(v);
	sq_pushobject(v, sc.vars);
	sq_setdelegate(v, -2);

	// Replace StellarContext's current variables table with the temporary table.
	HSQOBJECT vars;
	sq_getstackobj(v, -1, &vars);
	HSQOBJECT old_vars = sc.vars;
	sc.vars = vars;

/*	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_get(v, 1);
	sq_createinstance(v, -1);
	sqa::sqa_newobj(v, cs);*/
	linenum_t lastLine = 0;
	while(mode && !sc.scanner->eof()){
		TokenList argv;
		sc.buf = sc.scanner->nextLine(&argv);
		lastLine = sc.line;
		sc.line = long(sc.scanner->getLine());
		if(argv.size() == 0)
			continue;

		gltestp::dstring s = argv[0];
		if(!strcmp(s, "define")){
			sq_pushstring(sc.v, argv[1], -1);
			sq_pushfloat(sc.v, SQFloat(CoordSys::sqcalc(sc, argv[2], s)));
			sq_createslot(sc.v, -3);
			continue;
		}
		else if(!strcmp(s, "include")){
			// Concatenate current file's path and given file name to obtain a relative path.
			// Assume ASCII transparent encoding. (Note that it may not be the case if it's Shift-JIS or something,
			// but for the moment, just do not use multibyte characters for data file names.)
			const char *slash = strrchr(sc.fname, '\\');
			const char *backslash = strrchr(sc.fname, '/');
			const char *pathDelimit = slash ? slash : backslash;
			gltestp::dstring path;
			// Assume string before a slash or backslash a path
			if(pathDelimit)
				path.strncat(sc.fname, pathDelimit - sc.fname + 1);
			path.strcat(argv[1]);
			StellarFileLoadInt(path, cs, &sc);
			// TODO: Avoid recursive includes
			continue;
		}
		if(!strcmp(argv[0], "new"))
			s = argv[1], argv.pop_front();
		if(!strcmp(s, "astro") || !strcmp(s, "coordsys")){
			CoordSys *a = NULL;
			CoordSys *(*ctor)(const char *path, CoordSys *root) = CoordSys::classRegister.construct;
			if(argv.size() == 1){
				// If the block begins with keyword "astro", make Astrobj's constructor to default,
				// otherwise use CoordSys.
				if(!strcmp(s, "astro"))
					ctor = Astrobj::classRegister.construct;
				else
					ctor = CoordSys::classRegister.construct;
			}
			else{
				const CoordSys::CtorMap &cm = CoordSys::ctormap();
				for(CoordSys::CtorMap::const_reverse_iterator it = cm.rbegin(); it != cm.rend(); it++){
					ClassId id = it->first;
					if(!strcmp(id, argv[1])){
						ctor = it->second->construct;
						break;
					}
				}
			}
			if(4 <= argv.size())
				argv.pop_front();

			if(argv[1]){
				if((a = cs->findastrobj(argv[1])) && a->parent == cs){
					StellarContext sc2 = sc;
					std::stringstream sstr = std::stringstream(std::string(argv[2]));
					StellarStructureScanner ssc(&sstr, lastLine);
					sc2.scanner = &ssc;
					stellar_coordsys(sc2, a);
				}
				else
					a = NULL;
			}
			if(!a){
				if(argv.size() < 3){
					printf("%s(%ld): Lacking body block for %s %s\n", sc.fname, sc.line, argv[0].c_str(), argv[1].c_str());
					continue;
				}
				a = ctor(argv[1], cs);
				if(a){
					if(ctor){
						CoordSys *eis = a->findeisystem();
						if(eis)
							eis->addToDrawList(a);
					}
					StellarContext sc2 = sc;
					std::stringstream sstr = std::stringstream(std::string(argv[2]));
					StellarStructureScanner ssc(&sstr, lastLine);
					sc2.scanner = &ssc;
					stellar_coordsys(sc2, a);
				}
			}
		}
		else{
			std::vector<const char *> cargs(argv.size());
			for(int i = 0; i < argv.size(); i++)
				cargs[i] = argv[i];
			try{
				if(cs->readFile(sc, argv.size(), &cargs[0]));
				else{
		//			CmdPrintf("%s(%ld): Unknown parameter for CoordSys: %s", sc.fname, sc.line, s);
					printf("%s(%ld): Unknown parameter for %s: %s\n", sc.fname, sc.line, cs->classname(), s.operator const char *());
				}
			}
			catch(StellarError &e){
				printf("%s(%ld): Error %s: %s\n", sc.fname, sc.line, cs->classname(), e.what());
			}
		}
	}
//	sq_poptop(v);
	cs->readFileEnd(sc);
	sc.vars = old_vars; // Restore original variables table before returning
	return mode;
}

int Game::StellarFileLoadInt(const char *fname, CoordSys *root, StellarContext *prev_sc){
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
		std::ifstream fp(fname);
		if(!fp)
			return -1;
		int mode = 0;
		int inquote = 0;
		StellarContext sc;
		Universe *univ = root->toUniverse();
		CoordSys *cs = NULL;
		Astrobj *a = NULL;
		sc.fname = fname;
		sc.root = root;
		sc.line = 0;
		sc.fp = &fp;
		sc.scanner = new StellarStructureScanner(&fp);
//		sqa_init(&sc.v);

		HSQUIRRELVM v = sqvm;
		sc.v = v;

		if(prev_sc)
			sc.vars = prev_sc->vars; // If it's the first invocation, set the root table as delegate
		else{
			sq_pushroottable(v); // otherwise, obtain a delegate table from calling file
			sq_getstackobj(v, -1, &sc.vars);
		}

		stellar_coordsys(sc, root);

/*		CmdPrint("space.dat loaded.");*/
		fp.close();
		delete sc.scanner;
//		sq_close(sc.v);
//		CmdPrintf("%s loaded time %lg", fname, TimeMeasLap(&tm));
		printf("%s loaded time %lg\n", fname, TimeMeasLap(&tm));
	}
	return 0;
}

int Game::StellarFileLoad(const char *fname){
	return StellarFileLoadInt(fname, universe, nullptr);
}

double stellar_util::sqcalcd(StellarContext &sc, const char *str, const SQChar *context){
	StackReserver st(sc.v);
	gltestp::dstring dst = gltestp::dstring("return(") << str << ")";
	if(SQ_FAILED(sq_compilebuffer(sc.v, dst, dst.len(), context, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc compile error: " << context);
	sq_pushobject(sc.v, sc.vars);
	if(SQ_FAILED(sq_call(sc.v, 1, SQTrue, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc call error: " << context);
	SQFloat f;
	if(SQ_FAILED(sq_getfloat(sc.v, -1, &f)))
		throw StellarError(gltestp::dstring() << "sqcalc returned value not convertible to int: " << context);
	return f;
}

bool stellar_util::sqcalcb(StellarContext &sc, const char *str, const SQChar *context){
	StackReserver st(sc.v);
	gltestp::dstring dst = gltestp::dstring("return(") << str << ")";
	if(SQ_FAILED(sq_compilebuffer(sc.v, dst, dst.len(), context, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc compile error: " << context);
	sq_pushobject(sc.v, sc.vars);
	if(SQ_FAILED(sq_call(sc.v, 1, SQTrue, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc call error: " << context);
	SQBool f;
	if(SQ_FAILED(sq_getbool(sc.v, -1, &f)))
		throw StellarError(gltestp::dstring() << "sqcalc returned value not convertible to bool: " << context);
	return f != SQFalse;
}

int stellar_util::sqcalci(StellarContext &sc, const char *str, const SQChar *context){
	StackReserver st(sc.v);
	gltestp::dstring dst = gltestp::dstring("return(") << str << ")";
	if(SQ_FAILED(sq_compilebuffer(sc.v, dst, dst.len(), context, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc compile error: " << context);
	sq_pushobject(sc.v, sc.vars);
	if(SQ_FAILED(sq_call(sc.v, 1, SQTrue, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc call error: " << context);
	SQInteger f;
	if(SQ_FAILED(sq_getinteger(sc.v, -1, &f)))
		throw StellarError(gltestp::dstring() << "sqcalc returned value not convertible to int: " << context);
	return int(f);
}
