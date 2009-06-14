#include "clib/amat4.h"
#include "clib/avec3.h"

const double mat4identity[16] = {
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1,
};

/* Function conterparts of the macros are provided here.
  Inline expansion achieved by macros (or inline declaration in C++) is
  not always faster than function calls especially the macro is very large,
  because code size is critical for memory caching.
*/

void mat4mp(amat4_t mr, const amat4_t ma, const amat4_t mb){
	MAT4MP(mr, ma, mb);
}

void mat4vp(avec4_t vr, const amat4_t ma, const avec4_t vb){
	MAT4VP(vr, ma, vb);
}

void mat4vp3(avec3_t vr, const amat4_t ma, const avec3_t vb){
	MAT4VP3(vr, ma, vb);
}

void mat4dvp3(avec3_t vr, const amat4_t ma, const avec4_t vb){
	MAT4DVP3(vr, ma, vb);
}

void mat4tvp3(avec3_t vr, const amat4_t ma, const avec3_t vb){
	MAT4TVP3(vr, ma, vb);
}

void mat4tdvp3(avec3_t vr, const amat4_t ma, const avec4_t vb){
	MAT4TDVP3(vr, ma, vb);
}

void mat4translate(amat4_t mr, double sx, double sy, double sz){
	MAT4TRANSLATE(mr,sx,sy,sz);
}

void mat4translate3dv(amat4_t mr, avec3_t va){
	MAT4TRANSLATE(mr,va[0],va[1],va[2]);
}

void mat4translaten3dv(amat4_t mr, avec3_t va){
	MAT4TRANSLATE(mr,-va[0],-va[1],-va[2]);
}

void mat4rotx(amat4_t mr, const amat4_t ma, double p){
	MAT4ROTX(mr, ma, p);
}

void mat4roty(amat4_t mr, const amat4_t ma, double y){
	MAT4ROTY(mr, ma, y);
}

void mat4rotz(amat4_t mr, const amat4_t ma, double z){
	MAT4ROTZ(mr, ma, z);
}
