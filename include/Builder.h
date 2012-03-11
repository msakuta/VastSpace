#ifndef BUILDER_H
#define BUILDER_H
#include "Warpable.h"
#include "arms.h"
#include "glw/glwindow.h"
#include "Docker.h"

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

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
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	bool addBuild(const BuildStatic *);
	bool cancelBuild(int index){return cancelBuild(index, true);}
	void anim(double dt);
	virtual void doneBuild(Entity *child);
	virtual double getRU()const;
protected:
	bool cancelBuild(int index, bool recalc_time);
	double ru;
};

class EXPORT GLWbuild : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWbuild(const char *title, Builder *a) : st(title), builder(a), tabindex(0){
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
