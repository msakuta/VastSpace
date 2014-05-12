#ifndef GALAXY_FIELD_H
#define GALAXY_FIELD_H
#include "astrodef.h"
#include "CoordSys.h"

#define SLICES 256 //g_gs_slices
#define HDIV 256 //g_gs_stacks
#define SLICES1 (SLICES/8) // needs to be smaller
#define HDIV1 (HDIV/16)
#define SLICES2 (SLICES/16) // needs to be much smaller
#define HDIV2 (HDIV/16)
#define FIELD 256
#define HFIELD (FIELD/2)
#define FIELDZ 32
#define HFIELDZ (FIELDZ/2)
#define GALAXY_EXTENT (LIGHTYEAR_PER_KILOMETER*1.e5) // The galaxy has diameter of about one hundred kilolightyears
#define GALAXY_EPSILON 1e-3
#define GALAXY_DR 1e-3 /* dynamic range of galaxy lights */

typedef unsigned char GalaxyField[FIELD][FIELD][FIELDZ][4];

const GalaxyField *initGalaxyField();

Vec3d getGalaxyOffset();

/// \brief Returns density of stars at given point in the galaxy.
/// \param pos Position vector
/// \param cs Coordinate system which measures pos.  Passing NULL to this value make the function assume
///           that the position is in galaxy coordinates.
double galaxy_get_star_density_pos(const Vec3d &pos, const CoordSys *cs = NULL);

/// \brief Returns density of stars at the given viewpoint.
double galaxy_get_star_density(Viewer *vw);


#endif
