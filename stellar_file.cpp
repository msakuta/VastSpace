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
#include "entity.h"
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

/* Lagrange 1 point between two bodies */
LagrangeCS::LagrangeCS(const char *path, CoordSys *root){
	init(path, root);
	objs[0] = objs[1] = NULL;
}

void LagrangeCS::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << objs[0] << objs[1];
}

void LagrangeCS::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> objs[0] >> objs[1];
}

const char *Lagrange1CS::classname()const{return "Lagrange1CS";}

const ClassRegister<Lagrange1CS> Lagrange1CS::classRegister("Lagrange1");



void Lagrange1CS::anim(double dt){
	st::anim(dt);
	static int init = 0;
	double f, alpha;
	Vec3d delta, oldpos, pos1, pos2;

	if(!objs[0] || !objs[1])
		return;

	alpha = objs[1]->mass / (objs[0]->mass + objs[1]->mass);
	f = (1. - pow(alpha, 1. / 3.));

	pos1 = parent->tocs(objs[0]->pos, objs[0]->parent);
	pos2 = parent->tocs(objs[1]->pos, objs[1]->parent);

	oldpos = pos;
	delta = pos2 - pos1;
	pos = delta * f;
	pos += pos1;
	if(dt)
		velo = (pos - oldpos) * (1. / dt);
}

bool LagrangeCS::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(0);
	else if(!strcmp(s, "object1")){
		if(1 < argc){
			objs[0] = parent->findastrobj(argv[1]);
		}
	}
	else if(!strcmp(s, "object2")){
		if(1 < argc){
			objs[1] = parent->findastrobj(argv[1]);
		}
	}
	else
		return st::readFile(sc, argc, argv);
	return true;
}

const char *Lagrange2CS::classname()const{return "Lagrange2CS";}

const ClassRegister<Lagrange2CS> Lagrange2CS::classRegister("Lagrange2");

void Lagrange2CS::anim(double dt){
	st::anim(dt);
	static int init = 0;
	double f, alpha;
	Vec3d delta, oldpos, pos1, pos2;

	if(!objs[0] || !objs[1])
		return;

	alpha = objs[1]->mass / (objs[0]->mass + objs[1]->mass);
	f = (1. - pow(alpha, 1. / 3.));

	pos1 = parent->tocs(objs[0]->pos, objs[0]->parent);
	pos2 = parent->tocs(objs[1]->pos, objs[1]->parent);

	oldpos = pos;
	delta = pos2 - pos1;
	pos = delta * -1.;
	pos += pos1;
	if(dt)
		velo = (pos - oldpos) * (1. / dt);
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
/*	sqa::StackReserver sr(v);
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_get(v, 1);
	sq_createinstance(v, -1);
	sqa::sqa_newobj(v, cs);*/
	while(mode && fgets(sc.buf, MAX_LINE_LENGTH, sc.fp)){
		char *s = NULL, *ps;
		int argc, c = 0;
		char *argv[16], *post;
		sc.line++;
		argc = argtok(argv, sc.buf, &post, numof(argv));
		if(argc == 0)
			continue;
		if(!strcmp("}", argv[argc - 1])){
			argv[--argc] = NULL;
			mode = 0;
			if(argc == 0){
				break;
			}
		}
		if(!strcmp("{", argv[argc - 1])){
			argv[--argc] = NULL;
			if(argc == 0)
				continue;
		}
		s = argv[0];
		ps = argv[1];
		if(!strcmp(s, "define")){
			struct var *v;
			sc.vl->l = (var*)realloc(sc.vl->l, ++sc.vl->c * sizeof *sc.vl->l);
			v = &sc.vl->l[sc.vl->c-1];
			v->name = (char*)malloc(strlen(ps) + 1);
			strcpy(v->name, ps);
			v->type = var::CALC_D;
			v->value.d = 2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0.;
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
			char *pp;
			if(pp = strchr(s, '{')){
				*pp = '\0';
				constructor = Cons<CoordSys>;
				ps = s;
			}
			else{
				const CoordSys::CtorMap &cm = CoordSys::ctormap();
				for(CoordSys::CtorMap::const_reverse_iterator it = cm.rbegin(); it != cm.rend(); it++){
					ClassId id = it->first;
					if(!strcmp(id, ps)){
						ctor = it->second;
						c++;
						s = argv[c];
						ps = argv[c+1];
						break;
					}
				}
			}
			if(ps){
				if(pp = strchr(ps, '{'))
					*pp = '\0';
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
		else if(cs->readFile(sc, argc, (argv)));
		else if(s = strtok(s, " \t\r\n")){
//			CmdPrintf("%s(%ld): Unknown parameter for CoordSys: %s", sc.fname, sc.line, s);
			printf("%s(%ld): Unknown parameter for %s: %s\n", sc.fname, sc.line, cs->classname(), s);
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
//		sq_close(sc.v);
//		CmdPrintf("%s loaded time %lg", fname, TimeMeasLap(&tm));
		printf("%s loaded time %lg\n", fname, TimeMeasLap(&tm));
	}
	return 0;
}

int StellarFileLoad(const char *fname, CoordSys *root){
	return StellarFileLoadInt(fname, root, const_cast<varlist*>(calc_mathvars()));
}
