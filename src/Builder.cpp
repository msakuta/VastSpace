/** \file
 * \brief Implementation of Builder, GLWbuild and BuildCommand classes.
 */
#include "Builder.h"
#include "judge.h"
#include "serial_util.h"
#include "Player.h"
#include "antiglut.h"
#include "sqadapt.h"
#include "glw/popup.h"
#include "Application.h"
#include "StaticInitializer.h"
#include "SqInitProcess.h"
extern "C"{
#include <clib/mathdef.h>
}

/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289
#define SQRT2P2 (M_SQRT2/2.)

/// The multiplier on the build speed for debugging.
static double g_buildtimescale = 1.;


Builder::BuildRecipeList Builder::buildRecipes;

class EXPORT BuildRecipeProcess : public SqInitProcess{
public:
	Builder::BuildRecipeList &value;
	const SQChar *name;
	BuildRecipeProcess(Builder::BuildRecipeList &value, const char *name) : value(value), name(name){}
	virtual void process(HSQUIRRELVM)const;
};

void BuildRecipeProcess::process(HSQUIRRELVM v)const{
	Game *game = (Game*)sq_getforeignptr(v);

	sq_pushstring(v, name, -1); // root string

	if(SQ_FAILED(sq_get(v, -2))) // root obj
		throw SQFError(_SC("Build recipes are not defined!"));
	SQInteger len = sq_getsize(v, -1);
	if(-1 == len)
		throw SQFError(_SC("Build recipes size could not be acquired"));
	value.resize(len);
	for(int i = 0; i < len; i++){
		sq_pushinteger(v, i); // root obj i
		if(SQ_FAILED(sq_get(v, -2))) // root obj obj[i]
			continue;
		Builder::BuildRecipe &br = value[i];

		sq_pushstring(v, _SC("name"), -1); // root obj obj[i] "name"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].name
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr)))
				br.name = sqstr;
			else // Throw an error because there's no such thing like default name.
				throw SQFError("Build recipe name is not a string");
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(_SC("Build recipe missing name"));

		sq_pushstring(v, _SC("className"), -1); // root obj obj[i] "className"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].className
			const SQChar *sqstr;
			if(SQ_SUCCEEDED(sq_getstring(v, -1, &sqstr)))
				br.className = sqstr;
			else
				throw SQFError("Build recipe className is not a string");
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(_SC("Build recipe missing className"));

		sq_pushstring(v, _SC("buildtime"), -1); // root obj obj[i] "buildtime"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].buildtime
			SQFloat f;
			if(SQ_SUCCEEDED(sq_getfloat(v, -1, &f)))
				br.buildtime = f;
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(_SC("Build recipe missing buildtime"));

		sq_pushstring(v, _SC("cost"), -1); // root obj obj[i] "cost"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root obj obj[i] obj[i].cost
			SQFloat f;
			if(SQ_SUCCEEDED(sq_getfloat(v, -1, &f)))
				br.cost = f;
			sq_poptop(v); // root obj obj[i]
		}
		else
			throw SQFError(_SC("Build recipe missing cost"));

		sq_poptop(v); // root obj
	}
	sq_poptop(v); // root
}

/// \brief Loads Builder.nut initialization script on first call.
void Builder::init(){
	static bool initialized = false;
	if(!initialized){
		Game *game = application.clientGame ? application.clientGame : application.serverGame;
		SqInit(game->sqvm, _SC("scripts/Builder.nut"),
				BuildRecipeProcess(buildRecipes, "build"));
		initialized = true;
	}
}

void Builder::serialize(SerializeContext &sc){
	// Do NOT call Entity::serialize here because this class is a branch class.
	sc.o << ru; // Do not pass value of getRU(), it can contain errors.
	sc.o << build;
	sc.o << nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.o << buildque[i].num << buildque[i].st->name;
	}
}

void Builder::unserialize(UnserializeContext &sc){
	// Do NOT call Entity::unserialize here because this class is a branch class.
	sc.i >> ru;
	sc.i >> build;
	sc.i >> nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.i >> buildque[i].num;
		gltestp::dstring name;
		sc.i >> name;
		buildque[i].st = NULL;
		for(int j = 0; j < buildRecipes.size(); j++){
			if(buildRecipes[j].name == name){
				buildque[i].st = &buildRecipes[j];
				break;
			}
		}
		assert(buildque[i].st);
	}
}

bool Builder::addBuild(const BuildRecipe *st){
	if(numof(buildque) <= nbuildque && buildque[nbuildque-1].st != st)
		return false;
	if(buildque[nbuildque-1].st == st)
		buildque[nbuildque-1].num++;
	else{
		buildque[nbuildque].st = st;
		buildque[nbuildque].num = 1;
		if(0 == nbuildque)
			build = buildque[nbuildque].st->buildtime;
		nbuildque++;
	}
	return true;
}

bool Builder::cancelBuild(int index, bool recalc){
	if(index < 0 || nbuildque <= index)
		return false;
	if(!--buildque[index].num){
		::memmove(&buildque[index], &buildque[index+1], (--nbuildque - index) * sizeof *buildque);
	}
	if(recalc && index == 0 && nbuildque)
		build = buildque[0].st->buildtime;
	return true;
}

void Builder::doneBuild(Entity *child){}

void Builder::anim(double dt){
	dt *= g_buildtimescale;

	if(nbuildque){
		// If resource units gonna run out, stop construction then.
		if(ru < buildque[0].st->cost * (buildque[0].st->buildtime - build + dt) / buildque[0].st->buildtime){
			dt = ru * buildque[0].st->buildtime / buildque[0].st->cost + build - buildque[0].st->buildtime;
		}
	}

	while(nbuildque && build < dt){
//		ru -= buildque[0].st->cost * build / buildque[0].st->buildtime;
		// If we just subtract the actual RU value every frame, we could have it
		// not equal to RUs before start building subtracted by the unit cost,
		// because variable frame time's calculation errors build up.
		// So we don't really subtract the value until the construction is
		// complete.  Instead, we show the RUs partially subtracted by
		// calculating the amount every time requested.  See getRU().
		ru -= buildque[0].st->cost;
		dt -= build;
		Entity *created = Entity::create(buildque[0].st->className, this->w);
		assert(created);

		// Let's get along with our mother's faction.
		if(Entity *base = toEntity())
			created->race = base->race;

		doneBuild(created);
		cancelBuild(0, false);
		build += buildque[0].st->buildtime;
	}
	if(nbuildque){
//		ru -= buildque[0].st->cost * dt / buildque[0].st->buildtime;
		build -= dt;
	}
}

double Builder::getRU()const{
	if(nbuildque)
		return ru - buildque[0].st->cost * (buildque[0].st->buildtime - build) / buildque[0].st->buildtime;
	else
		return ru;
}

bool Builder::command(EntityCommand *com){
	if(BuildCommand *bc = InterpretCommand<BuildCommand>(com)){
		for(int i = 0; i < buildRecipes.size(); i++){
			if(bc->buildOrder == buildRecipes[i].name){
				addBuild(&buildRecipes[i]);
			}
		}
		return true;
	}
	return false;
}

/// \brief The class specific getter method for Squirrel scripts.
///
/// Note that it cannot be a native closure of Squirrel by its own.
/// It muse be called from one.
SQInteger Builder::sq_get(HSQUIRRELVM v, const SQChar *name){
	if(!strcmp(name, _SC("ru"))){
		sq_pushfloat(v, getRU());
		return 1;
	}
	else
		return 0;
}

/// \brief The class specific setter method for Squirrel scripts.
///
/// Note that it cannot be a native closure of Squirrel by its own.
/// It muse be called from one.
SQInteger Builder::sq_set(HSQUIRRELVM v, const SQChar *name){
	if(!strcmp(name, _SC("ru"))){
		SQFloat f;
		if(SQ_SUCCEEDED(sq_getfloat(v, 3, &f))){
			ru += f - getRU();
		}
	}
	else
		return SQ_ERROR;
}


IMPLEMENT_COMMAND(BuildCommand, "BuildCommand");

void BuildCommand::serialize(SerializeContext &sc){
	sc.o << buildOrder;
}

void BuildCommand::unserialize(UnserializeContext &sc){
	sc.i >> buildOrder;
}


#ifndef DEDICATED
static int select_tab(GLwindow *wnd, int ix, int iy, const char *s, int *selected, int mx, int my){
	int ix0 = ix;
	ix += 3;
	ix += 8 * strlen(s);
	ix += 3;
	*selected = ix0 < mx && mx < ix && iy < my + 12 && my + 12 < iy + 12;
	return ix;
}

int GLWbuild::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWbuild *p = this;
	int ind, sel0, sel1, ix;
	int fonth = (int)getFontHeight();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	ind = (my - 3 * 12) / 12;
	if((mbutton == GLUT_RIGHT_BUTTON || mbutton == GLUT_LEFT_BUTTON) && state == GLUT_UP || mbutton == GLUT_WHEEL_UP || mbutton == GLUT_WHEEL_DOWN){
		int num = 1, i;
#ifdef _WIN32
		if(HIBYTE(GetKeyState(VK_SHIFT)))
			num = 10;
#endif
		ix = select_tab(this, ix = 2, 2 * fonth, "Build", &sel0, mx, my);
		select_tab(this, ix, 2 * fonth, "Queue", &sel1, mx, my);
		if(sel0) p->tabindex = 0;
		if(sel1) p->tabindex = 1;
		if(p->tabindex == 0){
#if 1
			if(0 <= ind && ind < Builder::buildRecipes.size()){
				builder->addBuild(&Builder::buildRecipes[ind]);
				if(!game->isServer()){
					BuildCommand com(Builder::buildRecipes[ind].name);
					CMEntityCommand::s.send(builder->toEntity(), com);
				}
			}
#else
			if(0 <= ind && ind < 1) for(i = 0; i < num; i++) switch(ind){
				case 0:
/*					add_buildque(p->p, &scontainer_build);
					break;
				case 1:
					add_buildque(p->p, &worker_build);
					break;
				case 2:*/
					builder->addBuild(&sceptor_build);
					break;
/*				case 3:
					add_buildque(p->p, &beamer_build);
					break;
				case 4:
					if(mbutton == GLUT_RIGHT_BUTTON){
						int j;
						glwindow *wnd2;
						for(j = 0; j < p->p->narmscustom; j++) if(p->p->armscustom[j].builder == &assault_build)
							break;
						if(j == p->p->narmscustom){
							p->p->armscustom = realloc(p->p->armscustom, ++p->p->narmscustom * sizeof *p->p->armscustom);
							p->p->armscustom[j].builder = &assault_build;
							p->p->armscustom[j].c = numof(assault_hardpoints);
							p->p->armscustom[j].a = malloc(numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
							memcpy(p->p->armscustom[j].a, assault_arms, numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
						}
						wnd2 = ArmsShowWindow(AssaultNew, assault_build.cost, offsetof(assault_t, arms), p->p->armscustom[j].a, assault_hardpoints, numof(assault_hardpoints));
						wnd->modal = wnd2;
					}
					else
						add_buildque(p->p, &assault_build);
					break;*/
			}
#endif
		}
		else{
			if(builder->build != 0. && ind == 0 || ind == 1){
				builder->build = 0.;
				builder->cancelBuild(ind);
			}
/*			else if(2 <= ind && ind < pf->nbuildque + 2){
				int i;
				int ptr;
				ind -= 2;
				ptr = (pf->buildque0 + ind) % numof(pf->buildque);
				if(1 < pf->buildquenum[ptr])
					pf->buildquenum[ptr]--;
				else{
					for(i = ind; i < pf->nbuildque - 1; i++){
						int ptr = (pf->buildque0 + i) % numof(pf->buildque);
						int ptr1 = (pf->buildque0 + i + 1) % numof(pf->buildque);
						pf->buildque[ptr] = pf->buildque[ptr1];
						pf->buildquenum[ptr] = pf->buildquenum[ptr1];
					}
					pf->nbuildque--;
				}
			}*/
		}
		return 1;
	}
	return 0;
}

void GLWbuild::postframe(){
	if(builder && !builder->w)
		builder = NULL;
}

int cmd_build(int argc, char *argv[]){
	if(!application.clientGame)
		return 0;
	Player *player = application.clientGame->player;
	if(!player || player->selected.empty())
		return 0;
	Builder *pb = (*player->selected.begin())->getBuilder();
	if(pb)
		glwAppend(new GLWbuild(application.clientGame, "Build", pb));
	return 0;
}
#endif

static void register_build(){
#ifndef DEDICATED
	CmdAdd("buildmenu", cmd_build);
#endif
	CvarAdd("g_buildtimescale", &g_buildtimescale, cvar_double);
}

static StaticInitializer init(register_build);


