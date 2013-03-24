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
	sc.o << buildOrderGen;
	sc.o << buildStarted;
	sc.o << nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.o << buildque[i].orderId << buildque[i].num << buildque[i].st->name;
	}
}

void Builder::unserialize(UnserializeContext &sc){
	// Do NOT call Entity::unserialize here because this class is a branch class.
	sc.i >> ru;
	sc.i >> build;
	sc.i >> buildOrderGen;
	sc.i >> buildStarted;
	sc.i >> nbuildque;
	for(int i = 0; i < nbuildque; i++){
		sc.i >> buildque[i].orderId;
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

/// \brief Try to add build queue entry.
/// \returns true if successfully added to the queue. False if failed, for example, due to full queue.
bool Builder::addBuild(const BuildRecipe *st){
	if(buildque[nbuildque-1].st == st)
		buildque[nbuildque-1].num++;
	else if(nbuildque < numof(buildque)){
		buildque[nbuildque].st = st;
		buildque[nbuildque].num = 1;
		if(0 == nbuildque)
			build = buildque[nbuildque].st->buildtime;
		buildque[nbuildque].orderId = buildOrderGen++; // Generate unique id local to this Builder.
		nbuildque++;
	}
	else
		return false;
	return true;
}

/// \brief Overridable build queue entry start event handler.
/// \returns true if the Builder can start building the top entry in the build queue.
///
/// The default behaviour is to do nothing and return true.
bool Builder::startBuild(){
	return true;
}

/// \brief Overridable build queue entry finish event handler.
/// \returns true if successfully done building.
///
/// The default behaviour is to create an Entity.
bool Builder::finishBuild(){
	Entity *thise = toEntity();

	// We can make Builders in the client never create an Entity, but it's
	// not necessary since client-side Entity creation prediction is officially
	// supported.  We create the Entities in both sides, and if the client's
	// prediction fails, just synchronize to the server in the next update frame.
	if(thise){
		Entity *created = Entity::create(buildque[0].st->className, thise->w);
		assert(created);

		// Let's get along with our mother's faction.
		created->race = thise->race;

		doneBuild(created);
		return true;
	}
	return false;
}

/// \brief Cancels existing build order in the queue.
/// \param index The position in the queue that is demanded to be canceled.
/// \param recalc Whether to recalculate build time for the next entry.
/// \retruns true if canceled; false if invalid argument.
bool Builder::cancelBuild(int index, bool recalc){
	if(index < 0 || nbuildque <= index)
		return false;
	if(!--buildque[index].num){
		::memmove(&buildque[index], &buildque[index+1], (--nbuildque - index) * sizeof *buildque);
	}
	if(recalc && index == 0 && nbuildque)
		build = buildque[0].st->buildtime;
	buildStarted = false;
	return true;
}

/// \brief Overridable build done event handler.
///
/// This handler assumes the build recipe produces an Entity.
void Builder::doneBuild(Entity *child){}

void Builder::anim(double dt){
	dt *= g_buildtimescale;

	if(nbuildque){
		// Invoke build start event and find if it's really feasible.
		if(!buildStarted){
			if(!startBuild())
				return;
			else
				buildStarted = true;
		}

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

		if(finishBuild()){
			cancelBuild(0, false);
			build += buildque[0].st->buildtime;
		}
		else
			break; // What to do if fail to finishBuild()?
	}
	if(nbuildque){
//		ru -= buildque[0].st->cost * dt / buildque[0].st->buildtime;
		build -= dt;
	}
}

/// \returns Resource Units this Builder contains.
double Builder::getRU()const{
	if(nbuildque)
		return ru - buildque[0].st->cost * (buildque[0].st->buildtime - build) / buildque[0].st->buildtime;
	else
		return ru;
}

/// \brief Custom EntityCommand interpreter for Builder.
///
/// Builder is not really a subclass of Entity, so definition of this function does not override anything.
/// The integrator must call explicitly this function, which is already done in Entity::command().
bool Builder::command(EntityCommand *com){
	if(BuildCommand *bc = InterpretCommand<BuildCommand>(com)){
		for(int i = 0; i < buildRecipes.size(); i++){
			if(bc->buildOrder == buildRecipes[i].name){
				addBuild(&buildRecipes[i]);
			}
		}
		return true;
	}
	else if(BuildCancelCommand *bcc = InterpretCommand<BuildCancelCommand>(com)){
		for(int i = 0; i < nbuildque; i++){
			if(bcc->orderId == buildque[i].orderId){
				cancelBuild(i);
			}
		}
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


IMPLEMENT_COMMAND(BuildCommand, "Build");

BuildCommand::BuildCommand(HSQUIRRELVM v, Entity &e){
	const SQChar *name;
	if(SQ_SUCCEEDED(sq_getstring(v, 3, &name))){
		this->buildOrder = name;
	}
	else
		throw SQFError("Build command's argument is not a string");
}

void BuildCommand::serialize(SerializeContext &sc){
	sc.o << buildOrder;
}

void BuildCommand::unserialize(UnserializeContext &sc){
	sc.i >> buildOrder;
}


IMPLEMENT_COMMAND(BuildCancelCommand, "BuildCancel");

BuildCancelCommand::BuildCancelCommand(HSQUIRRELVM v, Entity &e){
	SQInteger orderId;
	if(SQ_SUCCEEDED(sq_getinteger(v, 3, &orderId))){
		this->orderId = orderId;
	}
	else
		throw SQFError("BuildCancel command's argument is not an integer");
}

void BuildCancelCommand::serialize(SerializeContext &sc){
	sc.o << orderId;
}

void BuildCancelCommand::unserialize(UnserializeContext &sc){
	sc.i >> orderId;
}


#ifndef DEDICATED
int cmd_build(int argc, char *argv[]){
	if(!application.clientGame)
		return 0;
	Player *player = application.clientGame->player;
	if(!player || player->selected.empty())
		return 0;
	for(Player::SelectSet::iterator it = player->selected.begin(); it != player->selected.end(); ++it){
		Builder *pb = (*player->selected.begin())->getBuilder();
		if(pb){
			glwAppend(new GLWbuild(application.clientGame, "Build", pb));
			return 0;
		}
	}
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


