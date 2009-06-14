#ifndef LIB_AQUAT_H
#define LIB_AQUAT_H

/* Quaternion arithmetics
 *   i * i = j * j = k * k = -1
 *   i * j = -(j * i) = k
 *   j * k = -(k * j) = i
 *   k * i = -(i * k) = j
 */

/* [0]i + [1]j + [2]k + [3] */
typedef double aquat_t[4];

#define VECTOQUAT(q,v) ((q)[0]=(v)[0],(q)[1]=(v)[1],(q)[2]=(v)[2],(q)[3]=0)

#define QUATZERO(q) ((q)[0]=(q)[1]=(q)[2]=(q)[3]=0)
#define QUATIDENTITY(q) ((q)[0]=(q)[1]=(q)[2]=0,(q)[3]=1)
extern const aquat_t quatidentity;

#define QUATCPY(r,a) ((r)[0]=(a)[0],(r)[1]=(a)[1],(r)[2]=(a)[2],(r)[3]=(a)[3])
#define QUATADD(r,a,b) ((r)[0]=(a)[0]+(b)[0],(r)[1]=(a)[1]+(b)[1],(r)[2]=(a)[2]+(b)[2],(r)[3]=(a)[3]+(b)[3])
#define QUATADDIN(r,a) ((r)[0]+=(a)[0],(r)[1]+=(a)[1],(r)[2]+=(a)[2],(r)[3]+=(a)[3])
#define QUATCNJ(r,a) ((r)[0]=-(a)[0],(r)[1]=-(a)[1],(r)[2]=-(a)[2],(r)[3]=(a)[3])
#define QUATCNJIN(r) ((r)[0]=-(r)[0],(r)[1]=-(r)[1],(r)[2]=-(r)[2],(r)[3]=(r)[3])
#define QUATSCALE(r,a,s) ((r)[0]=(a)[0]*(s),(r)[1]=(a)[1]*(s),(r)[2]=(a)[2]*(s),(r)[3]=(a)[3]*(s))
#define QUATSCALEIN(r,s) QUATSCALE(r,r,s)
#define QUATSLEN(a) ((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2]+(a)[3]*(a)[3])
#define QUATLEN(a) sqrt((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2]+(a)[3]*(a)[3])
#define QUATNORM(r,a) {double s; s = QUATLEN(a); (r)[0]=(a)[0]/s,(r)[1]=(a)[1]/s,(r)[2]=(a)[2]/s,(r)[3]=(a)[3]/s;}
#define QUATNORMIN(r) {double s; s = QUATLEN(r); (r)[0]/=s,(r)[1]/=s,(r)[2]/=s,(r)[3]/=s;}

#define QUATMUL(qr,qa,qb) (\
	(qr)[0]=(qa)[1]*(qb)[2]-(qa)[2]*(qb)[1]+(qa)[0]*(qb)[3]+(qa)[3]*(qb)[0],\
	(qr)[1]=(qa)[2]*(qb)[0]-(qa)[0]*(qb)[2]+(qa)[1]*(qb)[3]+(qa)[3]*(qb)[1],\
	(qr)[2]=(qa)[0]*(qb)[1]-(qa)[1]*(qb)[0]+(qa)[2]*(qb)[3]+(qa)[3]*(qb)[2],\
	(qr)[3]=-(qa)[0]*(qb)[0]-(qa)[1]*(qb)[1]-(qa)[2]*(qb)[2]+(qa)[3]*(qb)[3])

void quatmul(aquat_t qr, const aquat_t qa, const aquat_t qb);

/* Product with conjugate of another quaternion. */
#define QUATIMUL(qr,qa,qb) (\
	(qr)[0]=(qa)[1]*-(qb)[2]-(qa)[2]*-(qb)[1]+(qa)[0]*(qb)[3]+(qa)[3]*-(qb)[0],\
	(qr)[1]=(qa)[2]*-(qb)[0]-(qa)[0]*-(qb)[2]+(qa)[1]*(qb)[3]+(qa)[3]*-(qb)[1],\
	(qr)[2]=(qa)[0]*-(qb)[1]-(qa)[1]*-(qb)[0]+(qa)[2]*(qb)[3]+(qa)[3]*-(qb)[2],\
	(qr)[3]=-(qa)[0]*-(qb)[0]-(qa)[1]*-(qb)[1]-(qa)[2]*-(qb)[2]+(qa)[3]*(qb)[3])

void quatimul(aquat_t qr, const aquat_t qa, const aquat_t qb);

#endif
