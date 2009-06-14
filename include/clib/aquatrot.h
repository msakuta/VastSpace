#ifndef CLIB_AQUATROT_H
#define CLIB_AQUATROT_H
/* Methods to control quaternions as rotation expression.
  Not all quaternions need to express rotation, so I divided the
  header.
   For methods that treat quaternions as general number, refer
  clib/aquat.h. */

#include "clib/aquat.h"

typedef double amat4_t[16];
typedef double avec3_t[3];

/* Create rotation from axis and angle */
#define QUATROTATION(qr,p,sx,sy,sz) {\
	double len;\
	len = sin((p) / 2.);\
	qr[0] = len * (sx), qr[1] = len * (sy), qr[2] = len * (sz);\
	qr[3] = cos(p / 2.);\
}

void quatrotation(aquat_t qr, double p, double sx, double sy, double sz);
void quatrotationv(aquat_t qr, double p, const avec3_t axis);


/* Rotate given rotation with another rotation specified by axis and angle */
#define QUATROTATE(qr,qa,p,sx,sy,sz) {\
	aquat_t q1;\
	QUATROTATION(q1,p,sx,sy,sz);\
	QUATMUL(qr,qa,q1);\
}

void quatrotate(aquat_t qr, const aquat_t qa, double p, double sx, double sy, double sz);
void quatrotatev(aquat_t qr, const aquat_t qa, double p, const avec3_t axis);


/* Spherical Linear Interpolation. This calculation is complex enough to make
  it into a function. */
void quatslerp(aquat_t qr, const aquat_t qa, const aquat_t qb, const double f);


void quat2mat(amat4_t *mat, const aquat_t q);
void quat2imat(amat4_t *mat, const aquat_t pq);
void quatrot(avec3_t ret, const aquat_t q, const avec3_t src);
void quatirot(avec3_t ret, const aquat_t q, const avec3_t src);
void quatrotquat(aquat_t ret, const avec3_t v, const aquat_t src);
void quatdirection(aquat_t ret, const avec3_t dir);
void mat2quat(aquat_t q, const amat4_t m);
void imat2quat(aquat_t q, const amat4_t m);

#endif
