#ifndef LIB_AVEC4_H
#define LIB_AVEC4_H
#include <math.h>
/* avec4.h - Array Vector Utility in 3D + Affine Transformation Helper */
/* Contains definition of macros relating vector arithmetics.
  The vector should be expressed in array of double, to maintain
  compatibility with OpenGL commands. See also amat4.h for transformation
  matrix support. */
/* Warning: this utility is implemented as macro and C language's type check
  won't take effect, so be careful not to pass incorrect type argumets. */
/* Naming Conventions:
  The macros' arguments are named with a convention that
    'r' is the vector to store the results and whose initial values be overwritten,
	'a' or 'b' is the input vector whose values not be changed, and
	's' is the scalar double value. */

typedef double avec4_t[4];

#define VEC4NULL(r) ((r)[0]=(r)[1]=(r)[2]=0.,(r)[3]=1.)
#define VEC4CPY(r,a) ((r)[0]=(a)[0],(r)[1]=(a)[1],(r)[2]=(a)[2],(r)[3]=(a)[3])
#define VEC4ADD(r,a,b) ((r)[0]=(a)[0]+(b)[0],(r)[1]=(a)[1]+(b)[1],(r)[2]=(a)[2]+(b)[2],(r)[3]=(a)[3]+(b)[3])
#define VEC4ADDIN(a,b) ((a)[0]+=(b)[0],(a)[1]+=(b)[1],(a)[2]+=(b)[2],(a)[3]+=(b)[3])
#define VEC4SADD(r,b,s) ((r)[0]+=(b)[0]*(s),(r)[1]+=(b)[1]*(s),(r)[2]+=(b)[2]*(s),(r)[3]+=(b)[3]*(s))
#define VEC4SUB(r,a,b) ((r)[0]=(a)[0]-(b)[0],(r)[1]=(a)[1]-(b)[1],(r)[2]=(a)[2]-(b)[2],(r)[3]=(a)[3]-(b)[3])
#define VEC4SUBIN(a,b) ((a)[0]-=(b)[0],(a)[1]-=(b)[1],(a)[2]-=(b)[2],(a)[3]-=(b)[3])
#define VEC4SCALE(r,a,s) ((r)[0]=(a)[0]*(s),(r)[1]=(a)[1]*(s),(r)[2]=(a)[2]*(s),(r)[3]=(a)[3]*(s))
#define VEC4SCALEIN(r,s) ((r)[0]*=(s),(r)[1]*=(s),(r)[2]*=(s),(r)[3]*=(s))
#define VEC4SLEN(a) ((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2]+(a)[3]*(a)[3])
#define VEC4LEN(a) sqrt((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2]+(a)[3]*(a)[3])
#define VEC4SP(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
#define VEC4NORMIN(r) {double s; s = VEC4LEN(r); (r)[0]/=s,(r)[1]/=s,(r)[2]/=s,(r)[3]/=s;}
#define VEC4NORM(r,a) {double s; s = VEC4LEN(a); (r)[0]=(a)[0]/s,(r)[1]=(a)[1]/s,(r)[2]=(a)[2]/s,(r)[3]=(a)[3]/s;}
#define VEC4SDIST(a,b) (((a)[0]-(b)[0])*((a)[0]-(b)[0])+((a)[1]-(b)[1])*((a)[1]-(b)[1])+((a)[2]-(b)[2])*((a)[2]-(b)[2])+((a)[3]-(b)[3])*((a)[3]-(b)[3]))
#define VEC4DIST(a,b) sqrt(((a)[0]-(b)[0])*((a)[0]-(b)[0])+((a)[1]-(b)[1])*((a)[1]-(b)[1])+((a)[2]-(b)[2])*((a)[2]-(b)[2])+((a)[3]-(b)[3])*((a)[3]-(b)[3]))
#define VEC4EQ(a,b) ((a)[0]==(b)[0]&&(a)[1]==(b)[1]&&(a)[2]==(b)[2]&&(a)[3]==(b)[3])

#endif
