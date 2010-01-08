#ifndef SCARRY_H
#define SCARRY_H
#include "Warpable.h"
#include "arms.h"
#include "glwindow.h"

#define SCARRY_BUILDQUESIZE 8
#define SCARRY_SCALE .0010
#define SCARRY_BAYCOOL 2.

class Builder : public virtual Entity{
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

	double build;
	int nbuildque;
//	Entity *const base;
	virtual void doneBuild(Entity *child);
	BuildData buildque[SCARRY_BUILDQUESIZE];

	Builder() : build(0), nbuildque(0){}
	bool addBuild(const BuildStatic *);
	bool cancelBuild(int index);
	void anim(double dt);
};

class GLWbuild : public GLwindowSizeable{
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

class Dockable;
class Docker : public virtual Entity{
public:
	double baycool;
	Dockable *el;

	Docker() : baycool(0), el(NULL){}
	void anim(double dt);
	void dock(Dockable *);
	virtual bool undock(Dockable *);
};

class GLWdock : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLWdock(const char *title, Docker *a) : st(title), docker(a){
		flags |= GLW_CLOSE | GLW_COLLAPSABLE;
		xpos = 100;
		ypos = 200;
		width = 250;
		height = 100;
	}
	virtual void draw(GLwindowState &ws, double t);
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);
	virtual void postframe();

	Docker *docker;
};

extern const struct Builder::BuildStatic sceptor_build;

int cmd_build(int argc, char *argv[], void *pv);
int cmd_dockmenu(int argc, char *argv[], void *pv);

class Scarry : public Warpable, public Docker, public Builder{
public:
	typedef Warpable st; st *pst(){return static_cast<st*>(this);}

	Scarry(){init();}
	Scarry(WarField *w);
	void init();
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual double maxhealth()const;
	virtual double hitradius();
	virtual double maxenergy()const;
	virtual void anim(double dt);
	virtual Props props()const;
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual int popupMenu(PopupMenu &list);
	virtual int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual int armsCount()const;
	virtual const ArmBase *armsGet(int i)const;
	virtual double getRU()const;
	virtual Builder *getBuilder();
	virtual Docker *getDocker();
	virtual const maneuve &getManeuve()const;

protected:
	double ru;

	ArmBase **turrets;

	static hardpoint_static *hardpoints;
	static int nhardpoints;

	static const maneuve mymn;
	static hitbox hitboxes[];
	static const int nhitboxes;

	virtual void doneBuild(Entity *);
	virtual bool undock(Dockable *);
};

#if 0
/* Active Radar or Hyperspace Sonar.
   AR costs less energy but has limitation in speed (light), while
  HS propagates on hyperspace field where limitation of speed do not exist.
*/
struct hypersonar{
	avec3_t pos;
	double life;
	double rad;
	double speed;
	struct coordsys *cs;
	struct hypersonar *next;
	int type; /* 0 - Active Radar, 1 - Hyperspace Sonar. */
} *g_hsonar;
#endif


#endif
