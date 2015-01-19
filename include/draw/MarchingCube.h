/** \file 
 * \brief Definition of Marching Cubes algorithm interface
 */
#ifndef MARCHINGCUBE_H
#define MARCHINGCUBE_H
#include <cpplib/vec3.h>

namespace MarchingCube{

typedef struct {
   Vec3d p[3];
} TRIANGLE;

typedef struct {
   Vec3d p[8];
   double val[8];
} GRIDCELL;

extern const Vec3i vertexOffsets[8];

int Polygonise(const GRIDCELL &grid,double isolevel,TRIANGLE *triangles);

}

#endif
