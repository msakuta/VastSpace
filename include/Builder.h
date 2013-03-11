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
	struct BuildStatic{
		const char *name;
		Entity *(*create)(WarField *w, Builder *mother);
		double buildtime;
		double cost;
		int nhardpoints;
		const struct hardpoint_static *hardpoints;
	};
	struct BuildData{
		const BuildStatic *st;
		int num;
	};

	WarField *&w;
	double build;
	int nbuildque;
//	Entity *const base;
	BuildData buildque[SCARRY_BUILDQUESIZE];

	static const Builder::BuildStatic *builder0[];
	static const unsigned nbuilder0;
	Builder(WarField *&w) : w(w), build(0), nbuildque(0){}
	virtual Entity *toEntity() = 0; ///< It's almost like using RTTI and dynamic_cast.
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool addBuild(const BuildStatic *);
	bool cancelBuild(int index){return cancelBuild(index, true);}
	void anim(double dt);
	virtual void doneBuild(Entity *child);
	virtual double getRU()const;
	virtual bool command(EntityCommand *com);
	virtual SQInteger sq_get(HSQUIRRELVM v, const SQChar *name);
	virtual SQInteger sq_set(HSQUIRRELVM v, const SQChar *name);
protected:
	bool cancelBuild(int index, bool recalc_time);
	double ru;
};

/// \brief A command that instruct a Builder to start building a named recipe.
///
/// Can be sent from a client to the server.
struct EXPORT BuildCommand : public SerializableCommand{
	COMMAND_BASIC_MEMBERS(BuildCommand, EntityCommand);
	BuildCommand(gltestp::dstring buildOrder = "") : buildOrder(buildOrder){}
	BuildCommand(HSQUIRRELVM v, Entity &){}
	gltestp::dstring buildOrder;
	virtual void serialize(SerializeContext &);
	virtual void unserialize(UnserializeContext &);
};

#ifndef DEDICATED
/// \brief The build manager window.
class EXPORT GLWbuild : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWbuild(Game *game, const char *title, Builder *a) : st(game, title), builder(a), tabindex(0){
		flags |= GLW_CLOSE | GLW_COLLAPSABLE;
		xpos = 100;
		ypos = 100;
		width = 250;
		height = 100;
	}
	virtual void draw(GLwindowState &ws, double t);
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);
	virtual void postframe();

	Builder *builder;
	int tabindex;
protected:
	void progress_bar(double f, int width, int *piy);
	int draw_tab(int ix, int iy, const char *s, int selected);
};
#endif

#endif
