#ifndef LIB_AVEC_H
#define LIB_AVEC_H
#include <math.h>
/* avec3.h - Array Vector Utility in 3D */
/* Contains definition of macros relating vector arithmetics.
  The vector should be expressed in array of double, to maintain
  compatibility with OpenGL commands. */
/* Warning: this utility is implemented as macro and C language's type check
  won't take effect, so be careful not to pass incorrect type argumets. */
/* Naming Conventions:
  The macros' arguments are named with a convention that
    'r' is the vector to store the results and whose initial values be overwritten,
	'a' or 'b' is the input vector whose values not be changed, and
	's' is the scalar double value. */

typedef double avec3_t[3];

#define VECNULL(r) ((r)[0]=(r)[1]=(r)[2]=0.)
#define VECCPY(r,a) ((r)[0]=(a)[0],(r)[1]=(a)[1],(r)[2]=(a)[2])/*(*(struct vectorhack*)&(r)=*(struct vectorhack*)&(a))*/
#define VECADD(r,a,b) ((r)[0]=(a)[0]+(b)[0],(r)[1]=(a)[1]+(b)[1],(r)[2]=(a)[2]+(b)[2])
#define VECADDIN(a,b) ((a)[0]+=(b)[0],(a)[1]+=(b)[1],(a)[2]+=(b)[2])
#define VECSADD(r,b,s) ((r)[0]+=(b)[0]*(s),(r)[1]+=(b)[1]*(s),(r)[2]+=(b)[2]*(s))
#define VECSUB(r,a,b) ((r)[0]=(a)[0]-(b)[0],(r)[1]=(a)[1]-(b)[1],(r)[2]=(a)[2]-(b)[2])
#define VECSUBIN(a,b) ((a)[0]-=(b)[0],(a)[1]-=(b)[1],(a)[2]-=(b)[2])
#define VECSCALE(r,a,s) ((r)[0]=(a)[0]*(s),(r)[1]=(a)[1]*(s),(r)[2]=(a)[2]*(s))
#define VECSCALEIN(r,s) ((r)[0]*=(s),(r)[1]*=(s),(r)[2]*=(s))
#define VECSLEN(a) ((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2])
#define VECLEN(a) sqrt((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2])
#define VECSP(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VECNORMIN(r) {double s; s = VECLEN(r); (r)[0]/=s,(r)[1]/=s,(r)[2]/=s;}
#define VECNORM(r,a) {double s; s = VECLEN(a); (r)[0]=(a)[0]/s,(r)[1]=(a)[1]/s,(r)[2]=(a)[2]/s;}
#define VECVP(r,a,b) ((r)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],(r)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],(r)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VECSDIST(a,b) (((a)[0]-(b)[0])*((a)[0]-(b)[0])+((a)[1]-(b)[1])*((a)[1]-(b)[1])+((a)[2]-(b)[2])*((a)[2]-(b)[2]))
#define VECDIST(a,b) sqrt(((a)[0]-(b)[0])*((a)[0]-(b)[0])+((a)[1]-(b)[1])*((a)[1]-(b)[1])+((a)[2]-(b)[2])*((a)[2]-(b)[2]))
#define VECEQ(a,b) ((a)[0]==(b)[0]&&(a)[1]==(b)[1]&&(a)[2]==(b)[2])

extern const avec3_t avec3_000;
extern const avec3_t avec3_100;
extern const avec3_t avec3_010;
extern const avec3_t avec3_001;

#endif
