#ifndef CLIB_C_H
#define CLIB_C_H
/* Utility macros for C language.
  Almost all sources can use this header,
  so keep it minimum.
   Another header, on the other hand, will have
  little benefit by including this header.
*/

/* Count elements in an array. Note that this macro
  will not work to pointers. */
#define numof(a) (sizeof(a)/sizeof*(a))

/* Return sign of arithmetic values. */
#define signof(a) ((a)<0?-1:0<(a)?1:0)

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif


#endif
