#ifndef CLIB_CFLOAT_H
#define CLIB_CFLOAT_H
/* Floating arithmetics in C language */

/* Clip src between min and max. Useful alternation for MAX and MIN. */
extern double rangein(double src, double min, double max);

/* Let src value approach to dst. Maximum value to approach is specified
  by delta. If the boundary condition is periodic, wrap is the period and
  nearest direction is selected. Assuming wrap is greater than delta.
  Zero can be passed to wrap for non periodic boundary. */
extern double approach(double src, double dst, double delta, double wrap);

#endif
