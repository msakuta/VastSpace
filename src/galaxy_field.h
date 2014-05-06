#ifndef GALAXY_FIELD_H
#define GALAXY_FIELD_H
#include "astrodef.h"

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

extern GLubyte g_galaxy_field[FIELD][FIELD][FIELDZ][4];

#endif
