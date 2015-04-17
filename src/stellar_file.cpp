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
			else if(c == ';'){
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
				if(argv)
					*argv = tokens;
				return buf;
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










int StellarContext::parseCommand(TokenList &argv, CoordSys *cs){

	try{

		CommandMap::iterator it = commands->find(argv[0]);
		if(it != commands->end()){
			it->second(*this, argv);
		}
		else if(argv[0] == "if"){
			if(argv.size() < 3){
				printf("%s(%ld): Insufficient number of arguments to if command\n", this->fname, this->line);
				return -1;
			}
			else if(0 != stellar_util::sqcalcb(*this, argv[1], "if"))
				parseString(argv[2], cs, false, scanner->getLine());
			else if(5 <= argv.size() && argv[3] == "else") // With "else" keyword (which is ignored)
				parseString(argv[4], cs, false, scanner->getLine());
			else if(4 <= argv.size())
				parseString(argv[3], cs, false, scanner->getLine());
		}
		else if(argv[0] == "while"){
			if(argv.size() < 3){
				printf("%s(%ld): Insufficient number of arguments to while command\n", this->fname, this->line);
				return -1;
			}
			else while(0 != stellar_util::sqcalcb(*this, argv[1], "while"))
				parseString(argv[2], cs, false, scanner->getLine());
		}
		else if(argv[0] == "expr"){
			gltestp::dstring catstr;
			for(TokenList::iterator it = argv.begin() + 1; it != argv.end(); ++it)
				catstr += *it;
			CoordSys::sqcalc(*this, catstr, "expr");
		}
		else{

			std::vector<const char *> cargs(argv.size());
			for(int i = 0; i < argv.size(); i++)
				cargs[i] = argv[i];
			if(cs->readFile(*this, argv.size(), &cargs[0]));
			else
				printf("%s(%ld): Unknown parameter for %s: %s\n", this->fname, this->line, cs->classname(), argv[0].c_str());
		}
	}
	catch(StellarError &e){
		printf("%s(%ld): Error %s: %s\n", this->fname, this->line, cs->classname(), e.what());
	}
	return 0;
}

int StellarContext::parseBlock(CoordSys *cs){
	StellarContext &sc = *this;

	linenum_t lastLine = 0;
	while(!sc.scanner->eof()){
		TokenList argv;
		sc.buf = sc.scanner->nextLine(&argv);
		lastLine = sc.line;
		sc.line = long(sc.scanner->getLine());
		if(argv.size() == 0)
			continue;

		gltestp::dstring s = argv[0];
		if(!strcmp(s, "include")){
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
			parseFile(path, cs, &sc);
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
				if((a = cs->findastrobj(argv[1])) && a->parent == cs)
					parseString(argv[2], a, true, lastLine);
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
					parseString(argv[2], a, true, lastLine);
				}
			}
		}
		else{
			sc.parseCommand(argv, cs);
		}
	}
	return 0;
}

int StellarContext::parseString(const char *s, CoordSys *cs, bool enterCoordSys, linenum_t linenum){
	StellarContext sc2 = *this;
	std::stringstream sstr = std::stringstream(std::string(s));
	StellarStructureScanner ssc(&sstr, linenum);
	sc2.scanner = &ssc;
	if(enterCoordSys)
		return sc2.parseCoordSys(cs);
	else
		return sc2.parseBlock(cs);
}

int StellarContext::parseCoordSys(CoordSys *cs){
	StellarContext &sc = *this;
	typedef CoordSys *(Game::*CC)(const char *name, CoordSys *cs);

	const char *fname = sc.fname;
	const CoordSys *root = sc.root;
	int enable = 0;
	HSQUIRRELVM v = sc.game->sqvm;
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

	sc.parseBlock(cs);

//	sq_poptop(v);
	cs->readFileEnd(sc);
	sc.vars = old_vars; // Restore original variables table before returning
	return 0;
}

void StellarContext::scmd_define(StellarContext &sc, TokenList &argv){
	sq_pushstring(sc.v, argv[1], -1);
	sq_pushfloat(sc.v, SQFloat(CoordSys::sqcalc(sc, argv[2], argv[1])));
	sq_createslot(sc.v, -3);
}

/// Borrowed code from Squirrel library (squirrel3/squirrel/sqstring.h) which in turn borrowed
/// from Lua code.  The header file is Squirrel's internal header, so we cannot just include it
/// from this file without confronting dependency hell (which could be much worse when we update
/// Squirrel library).  Originally we considered using CRC32, but this code would be more efficient
/// for long strings.
inline SQHash _hashstr (const SQChar *s, size_t l)
{
		SQHash h = (SQHash)l;  /* seed */
		size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
		for (; l>=step; l-=step)
			h = h ^ ((h<<5)+(h>>2)+(unsigned short)*(s++));
		return h;
}

/// Return a hash for unordered_map of dstring.
static size_t hash_dstr(const gltestp::dstring &s){
	return _hashstr(s.c_str(), s.len());
}

int StellarContext::parseFile(const char *fname, CoordSys *root, StellarContext *prev_sc){
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
		std::ifstream fp(fname);
		if(!fp)
			return -1;
		int mode = 0;
		int inquote = 0;
		CommandMap commandMap(0, hash_dstr);
		commandMap["define"] = scmd_define;
		StellarContext sc;
		Universe *univ = root->toUniverse();
		CoordSys *cs = NULL;
		Astrobj *a = NULL;
		sc.game = root->getGame();
		sc.fname = fname;
		sc.root = root;
		sc.line = 0;
		sc.fp = &fp;
		sc.scanner = new StellarStructureScanner(&fp);
		sc.commands = &commandMap;
//		sqa_init(&sc.v);

		HSQUIRRELVM v = sc.game->sqvm;
		sc.v = v;

		if(prev_sc)
			sc.vars = prev_sc->vars; // If it's the first invocation, set the root table as delegate
		else{
			sq_pushroottable(v); // otherwise, obtain a delegate table from calling file
			sq_getstackobj(v, -1, &sc.vars);
		}

		sc.parseCoordSys(root);

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
	return StellarContext::parseFile(fname, universe, nullptr);
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
