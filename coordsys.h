#ifndef COORDSYS_H
#define COORDSYS_H
/* the concept is coordinate system tree. you can track the tree
  to find out where the jupiter's satellite is in the earth's 
  coordinates, for example. */

#include "viewer.h"

extern "C"{
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
}
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>
#include <vector>

#define CS_DELETE   2 /* marked as to be deleted */
#define CS_WARPABLE 4 /* can be targetted for warp destinaiton */
#define CS_SOLAR    8 /* designates solar system */
#define CS_EXTENT   0x10 /* the coordsys radius is large enough to hold all items within */
#define CS_ISOLATED 0x20 /* the parent coordsys is sparse enough to assure no sibling intersects */


class Viewer;
class WarField;
class Astrobj;
struct StellarContext;

/* node or leaf of coordinate system tree */
class CoordSys{
public:
	Vec3d pos;
	Vec3d velo;
	Quatd qrot;
	Vec3d omg;
	double rad; /* bounding sphere radius around origin, used for culling or conversion */
	CoordSys *parent;
	const char *name;
	const char *fullname;
	unsigned flags;

	/* These trivial variables are only used on drawing to enhance speed by caching
	  certain values. */
	enum vwflag{VW_POS = 1, VW_SDIST = 2, VW_DIST = 4, VW_SCALE = 8};
	int vwvalid; /* validity flags for vwpos and vwsdist */
	Vec3d vwpos; /* position in viewer's coordinates */
	double vwsdist; // squared distance
	double vwdist; /* distance from the viewer */
	double vwscale; /* apparent scale */

	WarField *w;
	CoordSys *next; /* list of siblings */
	int nchildren; /* unnecessary? */
	CoordSys *children/*[64]*/; /* starting pointer to list of children */
	typedef std::vector<CoordSys*> AOList;
	AOList aorder; /* ordered list of drawable objects belonging to this coordsys */
	int naorder; /* count of elements in aorder */

	CoordSys();
	CoordSys(const char *path, CoordSys *root);
	~CoordSys();
	void init(const char *path, CoordSys *root);

	virtual const char *classname()const; // returned string storage must be static
	virtual void anim(double dt);
	virtual void postframe();
	virtual void endframe();
	virtual void predraw(const Viewer *); // called just before draw method.
	virtual void draw(const Viewer *); // it's not const member function, altering contents is allowed.

	virtual bool belongs(const Vec3d &pos, const CoordSys *pos_cs)const;

	virtual bool readFileStart(StellarContext &); // enter block
	virtual bool readFile(StellarContext &, int argc, char *argv[]); // read a line from a text file
	virtual bool readFileEnd(StellarContext &); // exit block

	/* Definition of appropriate rotation. some coordinate systems like space colonies have
	  odd rules to rotate the camera. Default is 'trackball' type rotation. */
	virtual Quatd rotation(const Vec3d &pos, const Vec3d &pyr, const Quatd &srcq)const;

	// Cast to Astrobj, for distinguishing CoordSys and Astrobj from the children list.
	// Returns NULL if cast is not possible (i.e. CoordSys object).
	virtual Astrobj *toAstrobj();

	// Const version simply follows the behavior of non-const version.
	const Astrobj *toAstrobj()const{ return const_cast<CoordSys*>(this)->toAstrobj(); }

	Vec3d tocs(const Vec3d &src, const CoordSys *cs)const;

	/*
	   The data structure is associated with family tree because a child
	  coordinate system has only one parent, but one can have any number of
	  children.
	   The relationship of a parent and a child is stand on a fact that
	  both are aware of it.
	   We need to be able to find a given coordinate system's children or parent,
	  because there can be the case both are not direct ancestor of the other
	  (siblings, for example).
	   Although the coordiante system tree is relatively static data, maintaining
	  the bidirectional reference correctly is tough and bugsome by human hands.
	  So this function is here to make one's parent legitimize one as his child.
	  All we need is to specify coordinate systems' parent and pass them into this
	  function.
	   The returning pointer points to one in parent's memory, which points back to
	  me (precisely the returned storage can belong to one of siblings, but it's
	  considered parent's property). The returned pointer expires when another
	  sibling is added or deleted.
	*/
	CoordSys **legitimize_child();

	/* Adopt one as child which was originally another parent's */
	void adopt_child(CoordSys *newparent, bool retainPosition = true);

	/* convert position, velocity, rotation matrix into one coordinate system
	  to another. */
	Vec3d tocsv(const Vec3d src, const Vec3d srcpos, const CoordSys *cs)const;
	Quatd tocsq(const CoordSys *cs)const;
	Mat4d tocsm(const CoordSys *cs)const;
	Mat4d tocsim(const CoordSys *cs)const;

	/* Calculate where the position should belong to, based on extent radius of
	  coordinate systems (if one has no overrides to belonging definition function).
	  The center of extent bounding sphere is set to the coordinates'
	  origin, because it is more precise to do floating-point arithmetics near zero. */
	const CoordSys *belongcs(Vec3d &ret, const Vec3d &src, const CoordSys *cs)const;

	bool is_ancestor_of(const CoordSys *object)const;

	/* Search the tree with name. If there are some nodes of the same name, the newest offspring
	  is returned (Note that it's not nearest). */
	CoordSys *findcs(const char *name);

	/* Search the tree with path string. If there are some nodes of the same name, they are
	  distinguished by tree path. Node delimiter is slash (/).
	   Also, this function is expected to run faster compared to findcs() because it doesn't
	  search non-rewarding subtrees. */
	CoordSys *findcspath(const char *path);

	// partial path
	CoordSys *findcsppath(const char *path, const char *pathend);

	/* Get the absolute path string. */
	bool getpath(char *buf, size_t size)const;

	/* Find nearest Extent and Isolated system in ancestory. */
	CoordSys *findeisystem();

	Astrobj *findastrobj(const char *name);

	// This system must be a Extent and Isolated.
	bool addToDrawList(CoordSys *descendant);

	void startdraw();

	static void deleteAll(CoordSys **);

	Vec3d calcPos(const Viewer &vw);
	double calcSDist(const Viewer &vw);
	double calcDist(const Viewer &vw);
	double calcScale(const Viewer &vw);

private:
	int getpathint(char *buf, size_t size)const;
};

inline bool CoordSys::addToDrawList(CoordSys *descendant){
	for(AOList::iterator i = aorder.begin(); i != aorder.end(); i++) if(*i == descendant)
		return false;
	aorder.push_back(descendant);
	return true;
}

inline Vec3d CoordSys::calcPos(const Viewer &vw){
	if(vwvalid & VW_POS)
		return vwpos;
	vwvalid |= VW_POS;
	return vwpos = vw.cs->tocs(pos, parent);
}

inline double CoordSys::calcSDist(const Viewer &vw){
	if(vwvalid & VW_SDIST)
		return vwsdist;
	vwvalid |= VW_SDIST;
	return vwsdist = (vw.pos - calcPos(vw)).slen();
}

inline double CoordSys::calcDist(const Viewer &vw){
	if(vwvalid & VW_DIST)
		return vwdist;
	vwvalid |= VW_DIST;
	return vwdist = sqrt(calcSDist(vw));
}

inline double CoordSys::calcScale(const Viewer &vw){
	if(vwvalid & VW_SCALE)
		return vwscale;
	vwvalid |= VW_SCALE;
	return vwscale = rad * vw.gc->scale(calcPos(vw));
}

#endif
