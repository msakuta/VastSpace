#ifndef COORDSYS_H
#define COORDSYS_H
/* the concept is coordinate system tree. you can track the tree
  to find out where the jupiter's satellite is in the earth's 
  coordinates, for example. */

extern "C"{
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
}
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>

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
	int vwvalid; /* validity flags for vwpos and vwsdist */
	Vec3d vwpos; /* position in viewer's coordinates */
	double vwsdist; /* distance from the viewer */

	WarField *w;
	CoordSys *next; /* list of siblings */
	int nchildren; /* unnecessary? */
	CoordSys *children/*[64]*/; /* starting pointer to list of children */
	CoordSys **aorder; /* ordered list of drawable objects belonging to this coordsys */
	int naorder; /* count of elements in aorder */

	CoordSys();
	CoordSys(const char *path, CoordSys *root);
	~CoordSys();
	void init(const char *path, CoordSys *root);

	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void postframe();
	virtual void endframe();
	virtual void draw(const Viewer *);

	virtual bool belongs(const Vec3d &pos, const CoordSys *pos_cs)const;

	virtual bool readFile(StellarContext &, int argc, char *argv[]);

	/* Definition of appropriate rotation. some coordinate systems like space colonies have
	  odd rules to rotate the camera. Default is 'trackball' type rotation. */
	virtual Quatd rotation(const Vec3d &pos, const Vec3d &pyr, const Quatd &srcq)const;

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
	void adopt_child(CoordSys *newparent);

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

	/* Get the absolute path string. */
	bool getpath(char *buf, size_t size)const;

	/* Find nearest Extent and Isolated system in ancestory. */
	CoordSys *findeisystem();

	void startdraw();

	static void deleteAll(CoordSys **);

private:
	int getpathint(char *buf, size_t size)const;
};

#endif
