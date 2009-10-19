#ifndef ASTROCONST_H
#define ASTROCONST_H
/* Collection of astronomic constants. 
  Sources that are associated to space astronomy include
  this header to veridate the constants. */

#ifndef LIGHT_SPEED
#define LIGHT_SPEED 299792.458 /* km/s */
#endif

#define AU_PER_KILOMETER (149597870.691) /* ASTRONOMICAL UNIT */
#define LIGHTYEAR_PER_KILOMETER (LIGHT_SPEED*365*24*60*60)

#define DEF_UGC 6.67259e-20 /* [km^3 s^-2 kg^-1] Universal Gravitational Constant */
#define UGC (gravityfactor * DEF_UGC)
#define DAY_SECONDS (60*60*24)
#define MINUTE_SECONDS (60)
#define HOUR_SECONDS (60*60)
#define MONTH_SECONDS (DAY_SECONDS*30)
#define YEAR_SECONDS (DAY_SECONDS*365.2422)

#endif
