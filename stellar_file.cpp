#define _CRT_SECURE_NO_WARNINGS

#include "stellar_file.h"
//#include "TexSphere.h"
#include "serial_util.h"
#include "argtok.h"
#include "sqadapt.h"
extern "C"{
#include "calc/calc.h"
}
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

extern double gravityfactor;


teleport::teleport(CoordSys *acs, const char *aname, int aflags, const Vec3d &apos) : cs(acs), flags(aflags), pos(apos){
	name = new char[strlen(aname) + 1];
	strcpy(name, aname);
}

teleport::~teleport(){
	delete name;
}

void teleport::serialize(SerializeContext &sc){
//	st::serialize(sc);
	sc.o << cs;
	sc.o << name;
	sc.o << flags;
	sc.o << pos;
}

void teleport::unserialize(UnserializeContext &sc){
//	st::unserialize(sc);
	cpplib::dstring name;
	sc.i >> cs;
	sc.i >> name;
	sc.i >> flags;
	sc.i >> pos;

	this->name = strnewdup(name, name.len());
}




/// \brief The scanner class to interpret a Stellar Structure Definition file.
///
/// Can handle line comments, block comments and curly braces.
/// It would be even possible to suspend scanning per-character basis.
class StellarStructureScanner{
public:
	StellarStructureScanner(FILE *fp);
	cpplib::dstring nextLine(std::vector<cpplib::dstring>* = NULL);
	bool eof()const{return 0 != feof(fp);}
	long long getLine()const{return line;} /// Returns line number.
protected:
	FILE *fp;
	long long line;
	enum{Normal, LineComment, BlockComment, Quotes} state;
	cpplib::dstring buf;
	cpplib::dstring currentToken;
	std::vector<cpplib::dstring> tokens;
};

StellarStructureScanner::StellarStructureScanner(FILE *fp) : fp(fp), line(0), state(Normal){
}

/// Scan and interpret input stream, separate them into tokens, returns on end of line.
cpplib::dstring StellarStructureScanner::nextLine(std::vector<cpplib::dstring> *argv){
	buf = "";
	tokens.clear();
	currentToken = "";
	int c, lc = EOF;
	while((c = getc(fp)) != EOF){

		// Return the line
		if(c == '\n'){
			if(0 < currentToken.len()){
				tokens.push_back(currentToken);
				currentToken = "";
			}
			if(argv){
				*argv = tokens;
			}
			if(state == LineComment)
				state = Normal;
			line++;
			return buf;
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

			if(c == '{' || c == '}'){
				if(0 < currentToken.len()){
					tokens.push_back(currentToken);
					currentToken = "";
				}
				tokens.push_back(char(c));
				if(argv)
					*argv = tokens;
				return buf;
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
		}
		lc = c;
	}
				if(argv)
					*argv = tokens;
	return buf;
}







template<class T> CoordSys *Cons(const char *name, CoordSys *cs){
	return new T(name, cs);
};
typedef CoordSys *(*CC)(const char *name, CoordSys *cs);


#define MAX_LINE_LENGTH 512

/* stack state machine */
static int StellarFileLoadInt(const char *fname, CoordSys *root, struct varlist *vl);
static int stellar_astro(StellarContext *sc, Astrobj *a);

static int stellar_coordsys(StellarContext &sc, CoordSys *cs){
	const char *fname = sc.fname;
	const CoordSys *root = sc.root;
	struct varlist *vl = sc.vl;
	int mode = 1;
	int enable = 0;
	HSQUIRRELVM v = sc.v;
	cs->readFileStart(sc);
	sqa::StackReserver sr(v);
	sq_newtable(v);
	sq_setdelegate(v, -2);
/*	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_get(v, 1);
	sq_createinstance(v, -1);
	sqa::sqa_newobj(v, cs);*/
	while(mode && !sc.scanner->eof()){
//		fgets(sc.buf, MAX_LINE_LENGTH, sc.fp);
		std::vector<cpplib::dstring> argv;
		sc.buf = sc.scanner->nextLine(&argv);
//		char *s = NULL, *ps;
		int argc = int(argv.size()), c = 0;
//		char *argv[16], *post;
		sc.line = long(sc.scanner->getLine());
//		argc = argtok(argv, sc.buf, &post, numof(argv));
		if(argc == 0)
			continue;
		if(!strcmp("}", argv[argc - 1])){
			argv.pop_back();
			argc = argv.size();
			mode = 0;
			if(argc == 0){
				break;
			}
		}
		if(!strcmp("{", argv[argc - 1])){
			argv.pop_back();
			argc = argv.size();
			if(argc == 0)
				continue;
		}
		cpplib::dstring s = argv[0];
		cpplib::dstring ps = 1 < argv.size() ? argv[1] : "";
		if(!strcmp(s, "define")){
			struct var *v;
			sc.vl->l = (var*)realloc(sc.vl->l, ++sc.vl->c * sizeof *sc.vl->l);
			v = &sc.vl->l[sc.vl->c-1];
			v->name = (char*)malloc(strlen(ps) + 1);
			strcpy(v->name, ps);
			v->type = var::CALC_D;
			const char *av2 = argv[2];
			v->value.d = 2 < argc ? calc3(&av2, sc.vl, NULL) : 0.;
			sq_pushstring(sc.v, ps, -1);
			sq_pushfloat(sc.v, SQFloat(v->value.d));
			sq_createslot(sc.v, -3);
			continue;
/*			definition = 2, ps = NULL;*/
		}
		else if(!strcmp(s, "include")){
			StellarFileLoadInt(ps, cs, vl);
			continue;
		}
/*		else if(definition == 2){
			struct var *v;
			v = &sc.vl->l[sc.vl->c-1];
			v->value.d = calc3(&s, calc_mathvars(), NULL);
			s = NULL;
		}*/
		if(!strcmp(argv[0], "new"))
			s = argv[1], ps = argv[2], c++;
/*		if(s[0] == '/')
			cs2 = findcspath(root, s+1);
		else if(cs2 = findcs(root, s));
		else*/ if(!strcmp(s, "astro") || !strcmp(s, "coordsys") || /*argc == 1 &&*/ argv[argc-1][strlen(argv[argc-1])-1] == '{'){
			CoordSys *a = NULL;
			CC constructor = NULL;
			CoordSys *(*ctor)(const char *path, CoordSys *root) = NULL;
			const char *pp;
//			if(pp = strchr(s, '{')){
//				*pp = '\0';
			if(argv[argv.size()-1] == "}"){
				argv.pop_back();
				constructor = Cons<CoordSys>;
				ps = s;
			}
			else{
				const CoordSys::CtorMap &cm = CoordSys::ctormap();
				for(CoordSys::CtorMap::const_reverse_iterator it = cm.rbegin(); it != cm.rend(); it++){
					ClassId id = it->first;
					if(!strcmp(id, ps)){
						ctor = it->second->construct;
						c++;
						s = argv[c];
						ps = argv[c+1];
						break;
					}
				}
			}
			if(ps){
//				if(pp = strchr(ps, '{'))
//					*pp = '\0';
				if((a = cs->findastrobj(ps)) && a->parent == cs)
					stellar_coordsys(sc, a);
				else
					a = NULL;
			}
			if(!a && (a = (constructor ? constructor(ps, cs) : ctor ? ctor(ps, cs) : Cons<Astrobj>(ps, cs)))){
				if(!constructor && ctor){
//					a->init(argv[c+2], cs);
					CoordSys *eis = a->findeisystem();
					if(eis)
						eis->addToDrawList(a);
//					a->parent = cs;
//					a->legitimize_child();
				}
				stellar_coordsys(sc, a);
			}
		}
		else{
			const char *cargs[64];
			for(int i = 0; i < argv.size(); i++)
				cargs[i] = argv[i];
			if(cs->readFile(sc, argc, cargs));
			else{
	//			CmdPrintf("%s(%ld): Unknown parameter for CoordSys: %s", sc.fname, sc.line, s);
				printf("%s(%ld): Unknown parameter for %s: %s\n", sc.fname, sc.line, cs->classname(), s.operator const char *());
			}
		}
	}
//	sq_poptop(v);
	cs->readFileEnd(sc);
	return mode;
}

static int StellarFileLoadInt(const char *fname, CoordSys *root, struct varlist *vl){
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
		FILE *fp;
		char buf[MAX_LINE_LENGTH];
		int mode = 0;
		int inquote = 0;
		StellarContext sc;
		CoordSys *cs = NULL;
		Astrobj *a = NULL;
		sc.fname = fname;
		sc.root = root;
		sc.line = 0;
		sc.fp = fp = fopen(fname, "r");
		sc.scanner = new StellarStructureScanner(fp);
		if(!fp)
			return -1;
		sc.buf = buf;
		sc.vl = (varlist*)malloc(sizeof *sc.vl);
		sc.vl->c = 0;
		sc.vl->l = NULL;
		sc.vl->next = vl;
		sc.v = g_sqvm;
//		sqa_init(&sc.v);
		sq_pushroottable(sc.v);

		stellar_coordsys(sc, root);

/*		CmdPrint("space.dat loaded.");*/
		fclose(fp);
		if(sc.vl->l){
			for(unsigned i = 0; i < sc.vl->c; i++)
				free(sc.vl->l[i].name);
			free(sc.vl->l);
		}
		free(sc.vl);
		delete sc.scanner;
//		sq_close(sc.v);
//		CmdPrintf("%s loaded time %lg", fname, TimeMeasLap(&tm));
		printf("%s loaded time %lg\n", fname, TimeMeasLap(&tm));
	}
	return 0;
}

int StellarFileLoad(const char *fname, CoordSys *root){
	return StellarFileLoadInt(fname, root, const_cast<varlist*>(calc_mathvars()));
}
