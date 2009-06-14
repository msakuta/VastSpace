#ifndef LIB_AMAT3_H
#define LIB_AMAT3_H
#include "avec3.h"

typedef double amat3_t[9];

extern const amat3_t mat3identity;

struct smat3{
	amat3_t a;
};

#define MATCPY(mr,ma) (*(struct smat3*)(mr) = *(const struct smat3*)(ma))

#define MATIDENTITY(mr) (\
	(mr)[0]=(mr)[4]=(mr)[8]=1.,\
	(mr)[3]=(mr)[6]=\
	(mr)[1]=(mr)[7]=\
	(mr)[2]=(mr)[5]=0.)

#define MATTRANSPOSE(mr,ma) (\
	(mr)[0]=(ma)[0],(mr)[3]=(ma)[1],(mr)[6]=(ma)[2],\
	(mr)[1]=(ma)[3],(mr)[4]=(ma)[4],(mr)[7]=(ma)[5],\
	(mr)[2]=(ma)[6],(mr)[5]=(ma)[7],(mr)[8]=(ma)[8])

#define MATSCALE(mr,ma,sb) (\
	VECSCALE(&(mr)[0], &(ma)[0], sb),\
	VECSCALE(&(mr)[3], &(ma)[3], sb),\
	VECSCALE(&(mr)[6], &(ma)[6], sb))

#define MATSCALEIN(mr,sa) (\
	VECSCALEIN(&(mr)[0], sa),\
	VECSCALEIN(&(mr)[3], sa),\
	VECSCALEIN(&(mr)[6], sa))

#define MATADD(mr,ma,mb) (\
	VECADD(&(mr)[0], &(ma)[0], &(mb)[0]),\
	VECADD(&(mr)[3], &(ma)[3], &(mb)[3]),\
	VECADD(&(mr)[6], &(ma)[6], &(mb)[6]))

#define MATADDIN(mr,ma) (\
	VECADDIN(&(mr)[0], &(ma)[0]),\
	VECADDIN(&(mr)[3], &(ma)[3]),\
	VECADDIN(&(mr)[6], &(ma)[6]))

#define MATVP(vr,ma,vb) (\
	(vr)[0]=(ma)[0]*(vb)[0]+(ma)[3]*(vb)[1]+(ma)[6]*(vb)[2],\
	(vr)[1]=(ma)[1]*(vb)[0]+(ma)[4]*(vb)[1]+(ma)[7]*(vb)[2],\
	(vr)[2]=(ma)[2]*(vb)[0]+(ma)[5]*(vb)[1]+(ma)[8]*(vb)[2])

#define MATMP(mr,ma,mb) (\
	MATVP(&(mr)[0], ma, &(mb)[0]),\
	MATVP(&(mr)[3], ma, &(mb)[3]),\
	MATVP(&(mr)[6], ma, &(mb)[6]))

#define MATDET(ma) ((ma)[0]*(ma)[4]*(ma)[8] + (ma)[1]*(ma)[5]*(ma)[6] + (ma)[2]*(ma)[3]*(ma)[7]\
	 - (ma)[0]*(ma)[5]*(ma)[7] - (ma)[1]*(ma)[3]*(ma)[8] - (ma)[2]*(ma)[4]*(ma)[6])

#define MATINV(mr,ma) {\
	double det = (ma)[0]*(ma)[4]*(ma)[8] + (ma)[1]*(ma)[5]*(ma)[6] + (ma)[2]*(ma)[3]*(ma)[7]\
	 - (ma)[0]*(ma)[5]*(ma)[7] - (ma)[1]*(ma)[3]*(ma)[8] - (ma)[2]*(ma)[4]*(ma)[6];\
	int i, j;\
	for(i = 0; i < 3; i++) for(j = 0; j < 3; j++){\
		int i1 = (i+1)%3, j1 = (j+1)%3, i2 = (i+2)%3, j2 = (j+2)%3;\
/*	((ma)[4]*(ma)[8] - (ma)[5]*(ma)[7]) / det, (mr)[3] = ((ma)[2]*(ma)[7] - (ma)[1]*(ma)*/\
		(mr)[i*3+j] = ((ma)[i1*3+j1]*(ma)[i2*3+j2] - (ma)[i1*3+j2]*(ma)[i2*3+j1]) / det;\
	}\
}

void matinv(amat3_t mr, const amat3_t ma);

#define MAT3TO4(m4,m3) (\
	(m4)[3]=(m4)[7]=(m4)[11]=(m4)[12]=(m4)[13]=(m4)[14]=0.,\
	(m4)[15]=1.,\
	VECCPY(&(m4)[0],&(m3)[0]),\
	VECCPY(&(m4)[4],&(m3)[3]),\
	VECCPY(&(m4)[8],&(m3)[6]))

#define MAT4TO3(m3,m4) (\
	VECCPY(&(m3)[0],&(m4)[0]),\
	VECCPY(&(m3)[3],&(m4)[4]),\
	VECCPY(&(m3)[6],&(m4)[8]))

/* set the matrix so that it is equivalent to vector product */
#define MATSKEW(mr,va) (\
	(mr)[0] = (mr)[4] = (mr)[8] = 0.,\
	(mr)[1] = -(mr[3] = (va)[2]),\
	(mr)[5] = -(mr[7] = (va)[0]),\
	(mr)[6] = -(mr[2] = (va)[1]))

/*
(m00*1, m01*cp+m02*sp, m01*(-sp)+m02*cp)
(m10*1, m11*cp+m12*sp, m11*(-sp)+m12*cp)
(m20*1, m21*cp+m22*sp, m21*(-sp)+m22*cp)
*/
#define MATROTX(mr,ma,p) {\
	double sp, cp;\
	sp = sin(p), cp = cos(p);\
	(mr)[0] = (ma)[0], (mr)[3] = (ma)[3]*cp+(ma)[6]*sp, (mr)[6] = -(ma)[3]*sp+(ma)[6]*cp,\
	(mr)[1] = (ma)[1], (mr)[4] = (ma)[4]*cp+(ma)[7]*sp, (mr)[7] = -(ma)[4]*sp+(ma)[7]*cp,\
	(mr)[2] = (ma)[2], (mr)[5] = (ma)[5]*cp+(ma)[8]*sp, (mr)[8] = -(ma)[5]*sp+(ma)[8]*cp;\
}

/*
(m00*cy+m02*(-sy), m01*1, m00*sy+m02*cy)
(m10*cy+m12*(-sy), m11*1, m10*sy+m12*cy)
(m20*cy+m22*(-sy), m21*1, m20*sy+m22*cy)
*/
#define MATROTY(mr,ma,y) {\
	double sy, cy;\
	sy = sin(y), cy = cos(y);\
	(mr)[0] = (ma)[0]*cy+(ma)[6]*(-sy), (mr)[3] = (ma)[3], (mr)[6] = (ma)[0]*sy+(ma)[6]*cy,\
	(mr)[1] = (ma)[1]*cy+(ma)[7]*(-sy), (mr)[4] = (ma)[4], (mr)[7] = (ma)[1]*sy+(ma)[7]*cy,\
	(mr)[2] = (ma)[2]*cy+(ma)[8]*(-sy), (mr)[5] = (ma)[5], (mr)[8] = (ma)[2]*sy+(ma)[8]*cy;\
}

/*
(m00*cz+m01*sz, m00*-sz+m01*cz, m02)
(m10*cz+m11*sz, m10*-sz+m11*cz, m12)
(m20*cz+m21*sz, m20*-sz+m21*cz, m22)
*/
#define MATROTZ(mr,ma,y) {\
	double sz, cz;\
	sz = sin(y), cz = cos(y);\
	(mr)[0] = (ma)[0]*cz+(ma)[3]*(sz), (mr)[3] = (ma)[0]*-sz+(ma)[3]*cz, (mr)[6] = (ma)[6],\
	(mr)[1] = (ma)[1]*cz+(ma)[4]*(sz), (mr)[4] = (ma)[1]*-sz+(ma)[4]*cz, (mr)[7] = (ma)[7],\
	(mr)[2] = (ma)[2]*cz+(ma)[5]*(sz), (mr)[5] = (ma)[2]*-sz+(ma)[5]*cz, (mr)[8] = (ma)[8];\
}
extern void mat3rotz(amat3_t mr, const amat3_t ma, double y);


#endif
