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

void mat4transpose(amat4_t mr, const amat4_t ma){
	MAT4TRANSPOSE(mr, ma);
/*	int i, j;
	for(i = 0; i < 16; i++) for(j = 0; j < 4; j++)
		mr[i * 4 + j] = ma[j * 4 + i];*/
}

#define DOUBLESWAP(a,b) ((((unsigned long*)&(a))[0]^=((unsigned long*)&(b))[0],((unsigned long*)&(b))[0]^=((unsigned long*)&(a))[0],((unsigned long*)&(a))[0]^=((unsigned long*)&(b))[0]), \
	(((unsigned long*)&(a))[1]^=((unsigned long*)&(b))[1],((unsigned long*)&(b))[1]^=((unsigned long*)&(a))[1],((unsigned long*)&(a))[1]^=((unsigned long*)&(b))[1]))
#define MAT4TRANSPOSEIN(mr) (DOUBLESWAP(mr[1],mr[4]),DOUBLESWAP(mr[2],mr[8]),DOUBLESWAP(mr[6],mr[9]),DOUBLESWAP(mr[3],mr[12]),DOUBLESWAP(mr[7],mr[13]),DOUBLESWAP(mr[11],mr[14]))

void mat4transposein(amat4_t mr){
/*	amat4_t t;
	MAT4CPY(t, mr);
	MAT4TRANSPOSEIN(t);
	MAT4CPY(mr, t);*/
	MAT4TRANSPOSEIN(mr);
/*	int i, j;
	for(i = 0; i < 16; i++) for(j = 0; j < 4; j++)
		mr[i * 4 + j] = mr[j * 4 + i];*/
}

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

void mat4translate3dv(amat4_t mr, const avec3_t va){
	MAT4TRANSLATE(mr,va[0],va[1],va[2]);
}

void mat4translaten3dv(amat4_t mr, const avec3_t va){
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
