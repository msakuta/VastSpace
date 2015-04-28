#ifndef ASTRODEF_H
#define ASTRODEF_H
/** \file
 * \brief Collection of astronomic constants.
 *
 * Sources that are associated to space astronomy include
 * this header to validate the constants. */

#ifndef LIGHT_SPEED
#define LIGHT_SPEED 299792.458e3 /* m/s */
#endif

#define AU_PER_METER (149597870.691e3) /* ASTRONOMICAL UNIT */
#define AU_PER_KILOMETER (AU_PER_METER/1e3) /* ASTRONOMICAL UNIT */
#define LIGHTYEAR (9460730472580800.)
#define LIGHTYEAR_PER_KILOMETER (LIGHTYEAR/1e3)
#define PARSEC (3.08568e16)

#define DEF_UGC 6.67259e-11 /* [m^3 s^-2 kg^-1] Universal Gravitational Constant */
#define UGC (Universe::getGravityFactor(this) * DEF_UGC)
#define DAY_SECONDS (60*60*24)
#define MINUTE_SECONDS (60)
#define HOUR_SECONDS (60*60)
#define MONTH_SECONDS (DAY_SECONDS*30)
#define YEAR_SECONDS (DAY_SECONDS*365.2422)


#endif
