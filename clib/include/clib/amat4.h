#ifndef LIB_AMAT4_H
#define LIB_AMAT4_H
#include "avec3.h"
#include "avec4.h"
/* amat4.h - Array Matrix Utility in 3D + Affine Transformation Helper */
/* Contains definition of macros relating vector and matrix arithmetics.
  The vector should be expressed in array of double, to maintain
  compatibility with OpenGL commands. */
/* Naming Conventions:
  The macros' arguments are named with a convention that
    'r' is the object to store results and whose initial values be overwritten, and
	'a' or 'b' is the input object whose values not be changed.
  Following prefixes indicates accepting data types that
	'm' is amat4_t,
	'v' is avec4_t, and
	's' is scalar. */

typedef double amat4_t[16];

extern const double mat4identity[16];

struct smat4{
	amat4_t a;
};

#define MAT4CPY(mr,ma) (*(struct smat4*)(mr) = *(const struct smat4*)(ma))

#define MAT4IDENTITY(mr) (\
	(mr)[0]=(mr)[5]=(mr)[10]=(mr)[15]=1.,\
	(mr)[4]=(mr)[8]=(mr)[12]=\
	(mr)[1]=(mr)[9]=(mr)[13]=\
	(mr)[2]=(mr)[6]=(mr)[14]=\
	(mr)[3]=(mr)[7]=(mr)[11]=0.)

#define MAT4TRANSPOSE(mr,ma) (\
	(mr)[0]=(ma)[0],(mr)[4]=(ma)[1],(mr)[8]=(ma)[2],(mr)[12]=(ma)[3],\
	(mr)[1]=(ma)[4],(mr)[5]=(ma)[5],(mr)[9]=(ma)[6],(mr)[13]=(ma)[7],\
	(mr)[2]=(ma)[8],(mr)[6]=(ma)[9],(mr)[10]=(ma)[10],(mr)[14]=(ma)[11],\
	(mr)[3]=(ma)[12],(mr)[7]=(ma)[13],(mr)[11]=(ma)[14],(mr)[15]=(ma)[15])

#define MAT4VP(vr,ma,vb) (\
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[8]*(vb)[2]+(ma)[12]*(vb)[3],\
	(vr)[1]=(ma)[1]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[9]*(vb)[2]+(ma)[13]*(vb)[3],\
	(vr)[2]=(ma)[2]*(vb)[0]+(ma)[6]*(vb)[1]+(ma)[10]*(vb)[2]+(ma)[14]*(vb)[3],\
	(vr)[3]=(ma)[3]*(vb)[0]+(ma)[7]*(vb)[1]+(ma)[11]*(vb)[2]+(ma)[15]*(vb)[3])

extern void mat4vp(amat4_t mr, const amat4_t ma, const avec4_t vb);

#define MAT4MP(mr,ma,mb) (\
	MAT4VP(&(mr)[0], ma, &(mb)[0]),\
	MAT4VP(&(mr)[4], ma, &(mb)[4]),\
	MAT4VP(&(mr)[8], ma, &(mb)[8]),\
	MAT4VP(&(mr)[12], ma, &(mb)[12]))
/*	(mr)[0]=(ma)[0]*(mb)[0]+(ma)[4]*(mb)[1]+(ma)[8]*(mb)[2]+(ma)[12]*(mb)[3],\
	(mr)[4]=(ma)[1]*(mb)[4]+(ma)[5]*(mb)[5]+(ma)[9]*(mb)[6]+(ma)[13]*(mb)[7],\
	(mr)[8]=(ma)[2]*(mb)[8]+(ma)[6]*(mb)[9]+(ma)[10]*(mb)[11]+(ma)[14]*(mb)[12],\
	(mr)[12]=(ma)[3]*(mb)[0]+(ma)[7]*(vb)[1]+(ma)[11]*(vb)[2]+(ma)[15])*/

extern void mat4mp(avec4_t mr, const amat4_t ma, const amat4_t mb);

#define MAT4VP3(vr,ma,vb) (\
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[8]*(vb)[2]+(ma)[12],\
	(vr)[1]=(ma)[1]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[9]*(vb)[2]+(ma)[13],\
	(vr)[2]=(ma)[2]*(vb)[0]+(ma)[6]*(vb)[1]+(ma)[10]*(vb)[2]+(ma)[14])

extern void mat4vp3(avec4_t mr, const amat4_t ma, const avec3_t vb);

/* delta vector product */
#define MAT4DVP3(vr,ma,vb) (\
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[8]*(vb)[2],\
	(vr)[1]=(ma)[1]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[9]*(vb)[2],\
	(vr)[2]=(ma)[2]*(vb)[0]+(ma)[6]*(vb)[1]+(ma)[10]*(vb)[2])

extern void mat4dvp3(avec4_t mr, const amat4_t ma, const avec3_t vb);

/* product with transpose matrix, useful for obtaining inverse rotation without making another matrix to transpose */
#define MAT4TVP3(vr,ma,vb) (\
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[1]*(vb)[1]+(ma)[2]*(vb)[2]+(ma)[3],\
	(vr)[1]=(ma)[4]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[6]*(vb)[2]+(ma)[7],\
	(vr)[2]=(ma)[8]*(vb)[0]+(ma)[9]*(vb)[1]+(ma)[10]*(vb)[2]+(ma)[11])

extern void mat4tvp3(avec4_t mr, const amat4_t ma, const avec3_t vb);

/* inverse delta vector product */
#define MAT4TDVP3(vr,ma,vb) (\
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[1]*(vb)[1]+(ma)[2]*(vb)[2],\
	(vr)[1]=(ma)[4]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[6]*(vb)[2],\
	(vr)[2]=(ma)[8]*(vb)[0]+(ma)[9]*(vb)[1]+(ma)[10]*(vb)[2])

extern void mat4tdvp3(avec4_t mr, const amat4_t ma, const avec3_t vb);

#define MAT4TRANSLATE(mr,sx,sy,sz) (\
	(mr)[12]+=(mr)[0]*(sx)+(mr)[4]*(sy)+(mr)[8]*(sz),\
	(mr)[13]+=(mr)[1]*(sx)+(mr)[5]*(sy)+(mr)[9]*(sz),\
	(mr)[14]+=(mr)[2]*(sx)+(mr)[6]*(sy)+(mr)[10]*(sz),\
	(mr)[15]+=(mr)[3]*(sx)+(mr)[7]*(sy)+(mr)[11]*(sz))

#define MAT4SCALE(mr,sx,sy,sz) (\
	(mr)[0]*=(sx),(mr)[4]*=(sy),(mr)[8]*=(sz),\
	(mr)[1]*=(sx),(mr)[5]*=(sy),(mr)[9]*=(sz),\
	(mr)[2]*=(sx),(mr)[6]*=(sy),(mr)[10]*=(sz),\
	(mr)[3]*=(sx),(mr)[7]*=(sy),(mr)[11]*=(sz))

extern void mat4translate(amat4_t mr, double sx, double sy, double sz);
extern void mat4translate3dv(amat4_t mr, const avec3_t va);
extern void mat4translaten3dv(amat4_t mr, const avec3_t va);

/*
(m00*1, m01*cp+m02*sp, m01*(-sp)+m02*cp, m03*1)
(m10*1, m11*cp+m12*sp, m11*(-sp)+m12*cp, m13*1)
(m20*1, m21*cp+m22*sp, m21*(-sp)+m22*cp, m23*1)
(m30*1, m31*cp+m32*sp, m31*(-sp)+m32*cp, m33*1)
*/
#define MAT4ROTX(mr,ma,p) {\
	double sp, cp;\
	sp = sin(p), cp = cos(p);\
	(mr)[0] = (ma)[0], (mr)[4] = (ma)[4]*cp+(ma)[8]*sp, (mr)[8] = -(ma)[4]*sp+(ma)[8]*cp, (mr)[12] = (ma)[12],\
	(mr)[1] = (ma)[1], (mr)[5] = (ma)[5]*cp+(ma)[9]*sp, (mr)[9] = -(ma)[5]*sp+(ma)[9]*cp, (mr)[13] = (ma)[13],\
	(mr)[2] = (ma)[2], (mr)[6] = (ma)[6]*cp+(ma)[10]*sp, (mr)[10] = -(ma)[6]*sp+(ma)[10]*cp, (mr)[14] = (ma)[14],\
	(mr)[3] = (ma)[3], (mr)[7] = (ma)[7]*cp+(ma)[11]*sp, (mr)[11] = -(ma)[7]*sp+(ma)[11]*cp, (mr)[15] = (ma)[15];\
}
extern void mat4rotx(amat4_t mr, const amat4_t ma, double p);

/*
(m00*cy+m02*(-sy), m01*1, m00*sy+m02*cy, m03*1)
(m10*cy+m12*(-sy), m11*1, m10*sy+m12*cy, m13*1)
(m20*cy+m22*(-sy), m21*1, m20*sy+m22*cy, m23*1)
(m30*cy+m32*(-sy), m31*1, m30*sy+m32*cy, m33*1)
*/
#define MAT4ROTY(mr,ma,y) {\
	double sy, cy;\
	sy = sin(y), cy = cos(y);\
	(mr)[0] = (ma)[0]*cy+(ma)[8]*(-sy), (mr)[4] = (ma)[4], (mr)[8] = (ma)[0]*sy+(ma)[8]*cy, (mr)[12] = (ma)[12],\
	(mr)[1] = (ma)[1]*cy+(ma)[9]*(-sy), (mr)[5] = (ma)[5], (mr)[9] = (ma)[1]*sy+(ma)[9]*cy, (mr)[13] = (ma)[13],\
	(mr)[2] = (ma)[2]*cy+(ma)[10]*(-sy), (mr)[6] = (ma)[6], (mr)[10] = (ma)[2]*sy+(ma)[10]*cy, (mr)[14] = (ma)[14],\
	(mr)[3] = (ma)[3]*cy+(ma)[11]*(-sy), (mr)[7] = (ma)[7], (mr)[11] = (ma)[3]*sy+(ma)[11]*cy, (mr)[15] = (ma)[15];\
}
extern void mat4roty(amat4_t mr, const amat4_t ma, double y);

/*
(m00*cz+m01*sz, m00*-sz+m01*cz, m02, m03*1)
(m10*cz+m11*sz, m10*-sz+m11*cz, m12, m13*1)
(m20*cz+m21*sz, m20*-sz+m21*cz, m22, m23*1)
(m30*cz+m31*sz, m30*-sz+m31*cz, m32, m33*1)
*/
#define MAT4ROTZ(mr,ma,y) {\
	double sz, cz;\
	sz = sin(y), cz = cos(y);\
	(mr)[0] = (ma)[0]*cz+(ma)[4]*(sz), (mr)[4] = (ma)[0]*-sz+(ma)[4]*cz, (mr)[8] = (ma)[8], (mr)[12] = (ma)[12],\
	(mr)[1] = (ma)[1]*cz+(ma)[5]*(sz), (mr)[5] = (ma)[1]*-sz+(ma)[5]*cz, (mr)[9] = (ma)[9], (mr)[13] = (ma)[13],\
	(mr)[2] = (ma)[2]*cz+(ma)[6]*(sz), (mr)[6] = (ma)[2]*-sz+(ma)[6]*cz, (mr)[10] = (ma)[10], (mr)[14] = (ma)[14],\
	(mr)[3] = (ma)[3]*cz+(ma)[7]*(sz), (mr)[7] = (ma)[3]*-sz+(ma)[7]*cz, (mr)[11] = (ma)[11], (mr)[15] = (ma)[15];\
}
extern void mat4rotz(amat4_t mr, const amat4_t ma, double y);


#endif
