/** \file
 * \brief Definition of Builder and GLWbuild classes.
 *
 * Also defines BuildCommand that orders a Builder to start building an Entity.
 */
#ifndef BUILDER_H
#define BUILDER_H
#include "Warpable.h"
#include "arms.h"
#include "glw/glwindow.h"
#include "Docker.h"

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

/// \brief A class that virtually represent a construction line.
///
/// It is not a subclass of Entity.  It's intended to be inherited to an Entity-
/// derived class (multiple inheritance) to add the funciton to that class.
///
/// The user must explicitly call serialize(), unserialize(), anim() and command()
/// in the corresponding functions of their own to propagate those events to this class.
class EXPORT Builder : public Observable{
public:
	/// \brief The recipe to construct an Entity in a Builder.
	struct BuildRecipe{
		gltestp::dstring name; ///< Displayed name of this recipe.
		gltestp::dstring className; ///< Created Entity class name.
		double buildtime; ///< Time taken to fully build one, in seconds.
		double cost; ///< Cost in RUs.
	};
	typedef std::vector<BuildRecipe> BuildRecipeList;
	static BuildRecipeList buildRecipes; ///< The list of defined recipes.

	/// \brief A queued build entry. Refers to a BuildRecipe.
	struct BuildData{
		const BuildRecipe *st;
		int num; ///< Repeat count for this entry.
		int orderId; ///< To distinguish individual BuildData.
	};

protected:
	WarField *&w;
	double build;
	int buildOrderGen; ///< A generator for build queue orderIds in this Builder.
	int nbuildque;
//	Entity *const base;
	BuildData buildque[SCARRY_BUILDQUESIZE];
public:

	Builder(WarField *&w) : w(w), build(0), buildOrderGen(0), nbuildque(0){init();}
	virtual Entity *toEntity() = 0; ///< It's almost like using RTTI and dynamic_cast.
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool addBuild(const BuildRecipe *);
	bool cancelBuild(int index){return cancelBuild(index, true);}
	void anim(double dt);
	virtual void doneBuild(Entity *child);
	virtual double getRU()const;
	virtual bool command(EntityCommand *com);
	virtual SQInteger sq_get(HSQUIRRELVM v, const SQChar *name);
	virtual SQInteger sq_set(HSQUIRRELVM v, const SQChar *name);
protected:
	void init();
	bool cancelBuild(int index, bool recalc_time);
	double ru;

	friend class GLWbuild;
};

/// \brief A command that instruct a Builder to start building a named recipe.
///
/// Can be sent from a client to the server.
struct EXPORT BuildCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(BuildCommand, EntityCommand);
	BuildCommand(gltestp::dstring buildOrder = "") : buildOrder(buildOrder){}
	BuildCommand(HSQUIRRELVM v, Entity &);
	gltestp::dstring buildOrder;
	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

/// \brief A command that instruct a Builder to cancel building a queued recipe entry.
///
/// Can be sent from a client to the server.
struct EXPORT BuildCancelCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(BuildCancelCommand, EntityCommand);
	BuildCancelCommand(int orderId = 0) : orderId(orderId){}
	BuildCancelCommand(HSQUIRRELVM v, Entity &);
	int orderId;
	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

#ifndef DEDICATED
/// \brief The build manager window.
class EXPORT GLWbuild : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWbuild(Game *game, const char *title, Builder *a);
	virtual void draw(GLwindowState &ws, double t);
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);

	WeakPtr<Builder> builder;
	int tabindex;
protected:
	void progress_bar(double f, int width, int *piy);
	int draw_tab(int ix, int iy, const char *s, int selected);
};
#endif

#endif
