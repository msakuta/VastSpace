#ifndef COORDSYS_H
#define COORDSYS_H
/* the concept is coordinate system tree. you can track the tree
  to find out where the jupiter's satellite is in the earth's 
  coordinates, for example. */

#include "viewer.h"
#include "serial.h"

extern "C"{
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
}
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>
#include <cpplib/dstring.h>
#include <vector>

#define CS_DELETE   2 /* marked as to be deleted */
#define CS_WARPABLE 4 /* can be targetted for warp destinaiton */
#define CS_SOLAR    8 /* designates solar system */
#define CS_EXTENT   0x10 /* the coordsys radius is large enough to hold all items within */
#define CS_ISOLATED 0x20 /* the parent coordsys is sparse enough to assure no sibling intersects */


class Viewer;
class WarField;
class Astrobj;
class OrbitCS;
class Universe;
class Player;
struct StellarContext;
struct war_draw_data;

/** \brief A Coordinate system in space.
 *
 * Node or leaf of coordinate system tree.
 *
 * The coordinate system tree is much like directory tree in filesystems.
 * The node of tree can have zero or more child nodes and have one parent node.
 * Child nodes define translation and rotation relative to its parent node,
 * to express layered structure of systems.
 */
class EXPORT CoordSys : public Serializable{
public:
	typedef Serializable st; ///< Super Type

	Vec3d pos; ///< Position
	Vec3d velo; ///< Velocity
	Quatd rot; ///< Rotation in quaternion
	Vec3d omg; ///< Angular velocity
	double csrad; ///< Bounding sphere radius around origin, used for culling or conversion.
	CoordSys *parent; ///< Parent node
	const char *name; ///< Reference name
	const char *fullname; ///< Detailed name for displaying, etc.
	unsigned flags; ///< Utility flags

	/* These trivial variables are only used on drawing to enhance speed by caching
	  certain values. */
	enum vwflag{VW_POS = 1, VW_SDIST = 2, VW_DIST = 4, VW_SCALE = 8};
	int vwvalid; ///< validity flags for cached values.
	Vec3d vwpos; ///< cached position in viewer's coordinates
	double vwsdist; ///< cached squared distance
	double vwdist; ///< cached distance from the viewer
	double vwscale; ///< cached apparent scale

	WarField *w; ///< Bound WarField.
	CoordSys *next; ///< Pointer to next node of list of siblings.

	/// \brief Starting pointer to list of children.
	///
	/// All children are enumerated by following next of children.
	CoordSys *children/*[64]*/; 

	typedef std::vector<CoordSys*> AOList;

	/// Astrobj list. sorted every frame to draw stellar objects in order of distance from the viewpoint.
	///
	/// Ordered list of drawable objects belonging to this coordsys.
	AOList aorder;
	int naorder; ///< Count of elements in aorder.

	CoordSys();
	CoordSys(const char *path, CoordSys *root);
	virtual ~CoordSys();
	void init(const char *path, CoordSys *root);

	virtual const char *classname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &, void (Serializable::*)(SerializeContext &));
	virtual void anim(double dt); ///< Animate this system
	virtual void postframe(); ///< Clear pointers to dead objects.
	virtual void endframe(); ///< Terminating frame. Objects are deallocated in this function.
	virtual void predraw(const Viewer *); ///< called just before draw method.
	virtual void draw(const Viewer *); ///< Drawing method is not const member function, altering contents is allowed.

	/// Draw transparent parts. Only objects that use z-buffering to draw itself is necessary to
	/// override this function.
	virtual void drawtra(const Viewer *);

	virtual bool belongs(const Vec3d &pos)const; ///< Definition of 'inside' of the system.

	virtual bool readFileStart(StellarContext &); ///< Enter block in a stellar file.
	virtual bool readFile(StellarContext &, int argc, char *argv[]); /// Interpret a line in stellar file
	virtual bool readFileEnd(StellarContext &); ///< Exit block in a stellar file.

	/** Definition of appropriate rotation. some coordinate systems like space colonies have
	 * odd rules to rotate the camera. Default is 'trackball' type rotation. */
	virtual Quatd rotation(const Vec3d &pos, const Vec3d &pyr, const Quatd &srcq)const;

	/// Cast to Astrobj, for distinguishing CoordSys and Astrobj from the children list.
	/// Returns NULL if cast is not possible (i.e. CoordSys object).
	virtual Astrobj *toAstrobj();
	virtual OrbitCS *toOrbitCS();
	virtual Universe *toUniverse();

	/// Const version of toAstrobj simply follows the behavior of non-const version.
	const Astrobj *toAstrobj()const{ return const_cast<CoordSys*>(this)->toAstrobj(); }
	const OrbitCS *toOrbitCS()const{ return const_cast<CoordSys*>(this)->toOrbitCS(); };
	const Universe *toUniverse()const{ return const_cast<CoordSys*>(this)->toUniverse(); };

	/// recursively draws a whole tree of coordinate systems.
	/// note that this function is not a virtual function unlike draw(), which means
	/// it cannot be overridden and all relevant systems are assured to be drawn.
	void drawcs(const Viewer *);

	/**
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

	/// Called when the parent system is changed. At the time of calling this function, this->parent
	/// is already new one.
	/// \param oldParent can be NULL if it's the first time being attached to a system tree.
	virtual void onChangeParent(CoordSys *oldParent);

	/// Adopt one as child which was originally another parent's.
	void adopt_child(CoordSys *newparent, bool retainPosition = true);

	/// Converts position, velocity, rotation matrix into one coordinate system
	/// to another.
	Vec3d tocs(const Vec3d &src, const CoordSys *cs, bool delta = false)const;
	Vec3d tocsv(const Vec3d src, const Vec3d srcpos, const CoordSys *cs)const;
	Quatd tocsq(const CoordSys *cs)const;
	Mat4d tocsm(const CoordSys *cs)const;
	Mat4d tocsim(const CoordSys *cs)const;

	/** Calculate where the position should belong to, based on extent radius of
	  coordinate systems (if one has no overrides to belonging definition function).
	  The center of extent bounding sphere is set to the coordinates'
	  origin, because it is more precise to do floating-point arithmetics near zero. */
	const CoordSys *belongcs(Vec3d &ret, const Vec3d &src)const;

	bool is_ancestor_of(const CoordSys *object)const;

	/** Search the tree with name. If there are some nodes of the same name, the newest offspring
	  is returned (Note that it's not nearest). */
	CoordSys *findcs(const char *name);
	const CoordSys *findcs(const char *name)const{return const_cast<CoordSys*>(this)->findcs(name);}

	/** Search the tree with path string. If there are some nodes of the same name, they are
	  distinguished by tree path. Node delimiter is slash (/).
	   Also, this function is expected to run faster compared to findcs() because it doesn't
	  search non-rewarding subtrees. */
	CoordSys *findcspath(const char *path);
	const CoordSys *findcspath(const char *path)const{return const_cast<CoordSys*>(this)->findcspath(path);}

	/// partial path
	CoordSys *findcsppath(const char *path, const char *pathend);

	/// Get the absolute path string.
	bool getpath(char *buf, size_t size)const;
	cpplib::dstring getpath()const;

	/// Get the relative path.
	cpplib::dstring getrpath(const CoordSys *base)const;

	/// Find nearest Extent and Isolated system in ancestory.
	CoordSys *findeisystem();

	Astrobj *findastrobj(const char *name);

	Astrobj *findBrightest(const Vec3d &pos = vec3_000);
	const Astrobj *findBrightest(const Vec3d &pos = vec3_000)const{
		return const_cast<CoordSys*>(this)->findBrightest(pos);
	}

	// This system must be a Extent and Isolated.
	bool addToDrawList(CoordSys *descendant);

	void startdraw();
	void drawWar(war_draw_data *wd);

	static void deleteAll(CoordSys **);

	Vec3d calcPos(const Viewer &vw);
	double calcSDist(const Viewer &vw);
	double calcDist(const Viewer &vw);
	double calcScale(const Viewer &vw);

	// Called once at startup
	static bool registerCommands(Player *);
	static bool unregisterCommands(Player *);

	typedef std::map<ClassId, CoordSys *(*)(const char *path, CoordSys *root)> CtorMap;
	static const CtorMap &ctormap();
	class Static{
	public:
		ClassId id;
		CoordSys *(*construct)(const char *path, CoordSys *root);
		Serializable *(*stconstruct)();
		Static(ClassId id, CoordSys *(*construct)(const char *path, CoordSys *root), Serializable *(*stconstruct)())
			: id(id), construct(construct), stconstruct(stconstruct)
		{
			CoordSys::registerClass(*this);
		}
		~Static(){
			unregisterClass(id);
		}
	};
	template<typename T> static CoordSys *Conster(const char *path, CoordSys *root){
		return new T(path, root);
	}
	template<typename T> class Register : public Static{
	public:
		Register(ClassId id) : Static(id, Conster<T>, st::Conster<T>){}
	};
protected:
	static unsigned registerClass(Static &st);
	static void unregisterClass(ClassId);
	static const Register<CoordSys> classRegister;

private:
	int getpathint(char *buf, size_t size)const;
	int getpathint(cpplib::dstring &)const;
};

template<typename T> class ClassRegister : public T::Register<T>{
public:
	ClassRegister(ClassId id) : Register(id){}
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
	return vwscale = csrad * vw.gc->scale(calcPos(vw));
}

#endif
