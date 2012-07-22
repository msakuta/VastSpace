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

template<int id>
struct flags_t{
	typedef flags_t<id> tt;
	unsigned long ul;
	flags_t(unsigned long ul = 0) : ul(ul){
	}
/*	operator unsigned long(){
		return !!fine | !!shake << 1 | !!wind << 2;
	}*/
//	tefpol_flags_t &setFine(bool v = true){fine = v; return *this;}
/*	tefpol_flags_t &setThickness(unsigned v){
		assert(v < 4);
		ul &= ~(3 << ThickBit);
		ul |= v << ThickBit;
		return *this;
	}*/
/*	tefpol_flags_t &setRough(bool v = true){rough = v; return *this;}
	tefpol_flags_t &setReflect(bool v = true){reflect = v; return *this;}
	tefpol_flags_t &setMovable(bool v = true){movable = v; return *this;}
	tefpol_flags_t &setSkip(bool v = true){skip = v; return *this;}*/
	friend tt operator|(const tt &a, const tt &o){
		tt ret;
		ret.ul = a.ul | o.ul;
		return ret;
	}
	tt operator&(const tt &o)const{
		tt ret;
		ret.ul = ul & o.ul;
		return ret;
	}
	tt &operator|=(const tt &o){
		ul |= o.ul;
		return *this;
	}
	tt &operator&=(const tt &o){
		ul &= o.ul;
		return *this;
	}
	tt operator~()const{
		return ~ul;
	}
	bool operator==(const tt &o)const{
		return ul == o.ul;
	}
	operator bool()const{
		return !!ul;
	}
};

struct tefpol_flags_t : flags_t<0>{
	tefpol_flags_t(unsigned long ul) : flags_t<0>(ul){}
	tefpol_flags_t(flags_t<0> a) : flags_t<0>(a){}
	tefpol_flags_t getThickness()const{
		return ul & ThickMask;
	}
	static const int ThickBit = 7;
	static const int ThickMask = 7 << ThickBit;
};

#undef TEL3_HEADFORWARD
#define TEL3_HEADFORWARD flags_t<1>(1 << 1)
/*EXPORT extern const tefpol_flags_t TEP3_ROUGH;
EXPORT extern const tefpol_flags_t TEP3_REFLECT;
EXPORT extern const tefpol_flags_t TEP3_THICK;
EXPORT extern const tefpol_flags_t TEP3_THICKER;
EXPORT extern const tefpol_flags_t TEP3_THICKEST;
EXPORT extern const tefpol_flags_t TEP3_FAINT;*/


struct tent3d_fpol_list;
struct glcull;


EXPORT struct tent3d_fpol_list *NewTefpol3D(unsigned maxt, unsigned init, unsigned unit);
EXPORT void AddTefpol3D(struct tent3d_fpol_list *tepl, const Vec3d &pos, const Vec3d &velo, const Vec3d &gravity, const struct color_sequence *col, tefpol_flags_t flags, double life);
EXPORT struct tent3d_fpol *AddTefpolMovable3D(struct tent3d_fpol_list *tepl, const Vec3d &pos, const Vec3d &velo, const Vec3d &gravity, const struct color_sequence *col, tefpol_flags_t flags, double life);
EXPORT void MoveTefpol3D(struct tent3d_fpol *fpol, const Vec3d &pos, const Vec3d &velo, double life, int skip);
EXPORT void ImmobilizeTefpol3D(struct tent3d_fpol*);
EXPORT void AnimTefpol3D(struct tent3d_fpol_list *tell, double dt);
EXPORT void DrawTefpol3D(struct tent3d_fpol_list *tepl, const Vec3d &viewpoint, const struct glcull *);

#ifndef NPROFILE
#ifdef __cplusplus
extern "C"{
#endif
struct tent3d_fpol_debug{
	double animtefpol, drawtefpol;
	size_t so_tefpol3_t, so_tevert3_t;
	unsigned tefpol_c;
	unsigned tefpol_m;
	unsigned tefpol_s;
	unsigned tevert_c;
	unsigned tevert_s;
};
const struct tent3d_fpol_debug *Tefpol3DDebug(const struct tent3d_fpol_list *);
#ifdef __cplusplus
}
#endif
#endif


#endif
