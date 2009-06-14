#ifndef MATHDEF_C_H
#define MATHDEF_C_H
/* Utility macros for C language in mathematics.
  Almost all sources can use this header,
  so keep it minimum.
   Another header, on the other hand, will have
  little benefit by including this header.
*/

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_E
#define M_E 2.7182818284590452353602874713527
#endif

/* This one is not essential constant, but sometimes
  useful treating rotational arithmetics. */
#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887242097
#endif

#ifndef deg_per_rad
#define deg_per_rad (360./2./M_PI)
#endif


#endif
