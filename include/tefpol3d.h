/** \file
 * \brief Definition of Tefpol and TefpolList classes.
 */
#ifndef TEFPOL3D_H
#define TEFPOL3D_H
#include "tent3d.h"

#ifdef __cplusplus
extern "C"{
#endif
#include <clib/colseq/color.h>
#include <clib/colseq/cs.h>
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <cpplib/vec3.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>
#include <cpplib/gl/cullplus.h>
#endif
#ifndef NPROFILE
#include <stddef.h>
#endif
#include "export.h"

/* tefpol_flags_t */
#define TEP_FINE        (1<<0) /* finer granularity for graduation */
#define TEP_SHAKE       (1<<1) /* randomly shift vertices per rendering */
#define TEP_WIND        (1<<2) /* wake of polyline will wind */
#define TEP_RETURN      (1<<3) /* lines gone beyond border will appear at opposide edge */
#define TEP_DRUNK       (1<<4) /* add random velocity to the head per step */
#define TEP3_REFLECT     tefpol_flags_t(1<<5) /* whether reflect on border */
#define TEP3_ROUGH		tefpol_flags_t(1<<6) /* use less vertices */
#define TEP3_THICK       tefpol_flags_t(1<<7) /* enthicken polyline to 2 pixels wide */
#define TEP3_THICKER     tefpol_flags_t(2<<7) /* enthicken polyline to 3 pixels wide */
#define TEP3_THICKEST    tefpol_flags_t(3<<7) /* enthicken polyline to 4 pixels wide */
#define TEP3_FAINT       tefpol_flags_t(4<<7) /* have volume but dim */
#define TEP3_HEAD        (1<<10) /* append head graphic */
#define TEP_ADDBLEND    (0<<14) /* additive; default */
#define TEP_SOLIDBLEND  (1<<14) /* transparency mode */
#define TEP_ALPHABLEND  (2<<14) /* alpha indexed */
#define TEP_SUBBLEND    (3<<14) /* subtractive */
/* teline's forms can be used as well. */

struct glcull;

/// Namespace provided to avoid potential collision with tent2d.
namespace tent3d{

typedef unsigned long tefpol_flags_t;
struct TefpolVertex;
struct TefpolList;

/// \brief Temporary Entity Follow POlyLine
///
/// A set of automatically decaying line segments.
struct EXPORT Tefpol{
public:
	/// \brief Move position of this Tefpol's generating point.
	void move(const Vec3d &pos, const Vec3d &velo, const double life, int skip);

	/// \brief Mark the Tefpol so that it will never move again. If the trailing polyline is exhausted, it will be deleted.
	void immobilize();

	/// \brief Animate this object
	/// \param dt Delta-time of the frame.
	void anim(double dt);

	void setTexture(unsigned long u){texture = u;}

protected:
	Vec3d pos;
	Vec3d velo;
	double life;
	Vec3d grv;
	const color_sequence *cs;
	tefpol_flags_t flags; /* entity options */
	unsigned cnt; /* generated trail node count */
	unsigned long texture; ///< Texture list
	TefpolVertex *head, *tail; /* polyline's */
	Tefpol *next;
	friend struct TefpolList;
	static const unsigned Movable = 1<<16;
	static const unsigned Skip = 1<<17;
};

/// \brief Polyline vertex in Tefpol.
///
/// A Tefpol can have arbitrary number of vertices in the vertices buffer.
typedef struct TefpolVertex{
	Vec3d pos;
	float dt;
	struct TefpolVertex *next;
} tevert_t;


/// \brief Tefpol object list.
///
/// It's almost like std::list or std::deque of Tefpol, but implemented manually for
/// optimization. Honestly, the fact it have originally written in C is the reason why
/// this is not a STL container.
struct EXPORT TefpolList{
protected:
	struct te_fpol_extra_list{
		struct te_fpol_extra_list *next;
		Tefpol l[1]; /* variable */
	};
	unsigned m;
	unsigned ml;
	unsigned mex;
	Tefpol *l;
	Tefpol *lfree, *lactv, *last;
	te_fpol_extra_list *ex;
	unsigned bs; /* verbose buffer size */
public:
	TefpolList(unsigned maxt, unsigned init, unsigned unit);
	~TefpolList();
	void addTefpol(const Vec3d &pos, const Vec3d &velo, const Vec3d &gravity, const struct color_sequence *col, tefpol_flags_t flags, double life);
	Tefpol *addTefpolMovable(const Vec3d &pos, const Vec3d &velo, const Vec3d &gravity, const struct color_sequence *col, tefpol_flags_t flags, double life);
	void anim(double dt);
	void draw(const Vec3d &viewpoint, const glcull *glc);
#ifndef NPROFILE
	struct tent3d_fpol_debug{
		double animtefpol, drawtefpol;
		size_t so_tefpol3_t, so_tevert3_t;
		unsigned tefpol_c;
		unsigned tefpol_m;
		unsigned tefpol_s;
		unsigned tevert_c;
		unsigned tevert_s;
	};
	tent3d_fpol_debug debug;
	const tent3d_fpol_debug *getDebug()const{return &debug;}
#endif
protected:
	Tefpol *allocTefpol3D(const Vec3d &pos, const Vec3d &velo,
			   const Vec3d &grv, const color_sequence *col, tefpol_flags_t f, double life);
	void freeTefpolExtra(te_fpol_extra_list*);
};


}

using namespace tent3d;


inline void tent3d::TefpolList::addTefpol(const Vec3d &pos, const Vec3d &velo,
			   const Vec3d &grv, const color_sequence *col, tefpol_flags_t f, double life)
{
	allocTefpol3D(pos, velo, grv, col, f & ~(Tefpol::Movable | Tefpol::Skip), life);
}

inline tent3d::Tefpol *tent3d::TefpolList::addTefpolMovable(const Vec3d &pos, const Vec3d &velo,
			   const Vec3d &grv, const color_sequence *col, tefpol_flags_t f, double life)
{
	return allocTefpol3D(pos, velo, grv, col, f | Tefpol::Movable, life);
}


#endif
