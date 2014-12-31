#ifndef PERLINNOISE3D_H
#define PERLINNOISE3D_H
/** \file
 * \brief Defines Perlin Noise in 3D generator classes and functions.
 */

#include <stdio.h>
extern "C"{
#include <clib/rseq.h>
//#include <clib/random/mt19937ar.h>
#include <clib/timemeas.h>
}
//#include <clib/random/tinymt32.h>
#include <cpplib/RandomSequence.h>
#include "SignModulo.h"
#include "perlinNoise.h"


namespace PerlinNoise{


/// \brief Callback object that receives result of perlin_noise
///
/// We do this because not all application needs temporary memry to store the result.
class PerlinNoiseCallback3D{
public:
	virtual void operator()(float, int ix, int iy, int iz) = 0;
};

/// \brief Callback object that will assign noise values to float 3-d array field.
template<int CELLSIZE>
class FieldAssign3D/* : public PerlinNoiseCallback3D*/{
	typedef float (&fieldType)[CELLSIZE][CELLSIZE][CELLSIZE];
	fieldType field;
public:
	FieldAssign3D(fieldType field) : field(field){}
	void operator()(int f, double maxi, int ix, int iy, int iz){
		field[ix][iy][iz] = float(f / maxi);
	}
};

/// \brief Parameters given to perlin_noise_3D(), sharing common parameters with 2D.
struct PerlinNoiseParams3D : PerlinNoiseParams{
	int zofs;

	PerlinNoiseParams3D(
		long seed = 0,
		double persistence = 0.5,
		long octaves = 4,
		int xofs = 0,
		int yofs = 0,
		int zofs = 0)
		: PerlinNoiseParams(seed, persistence, octaves, xofs, yofs), zofs(zofs)
	{
	}
};

template<int CELLSIZE>
inline void perlin_noise_3D(long seed, PerlinNoiseCallback &callback, int xofs = 0, int yofs = 0, int zofs = 0){
	perlin_noise_3D<CELLSIZE>(PerlinNoiseParams3D(seed, 0.5, xofs, yofs, zofs), callback);
}

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/// \brief Generate Perlin Noise and return it through callback.
/// \param CELLSIZE The cell size in voxels.
/// \param Callback The type for callback object. Can be a function or a functionoid.
/// \param param Parameters to generate the noise.
/// \param callback Callback object to receive the result.
template<int CELLSIZE, typename Callback>
void perlin_noise_3D(const PerlinNoiseParams3D &param, Callback &callback){
	static const int baseMax = 255;

	struct Random{
/*		RandomSequence rs;
		Random(long seed, long x, long y, long z) : rs(seed ^ x ^ (z << 16), y ^ ((unsigned long)z >> 16)){
		}
		unsigned long next(){
			unsigned long u = rs.next();
			return (u >> 8) & 0xf0 + (u & 0xf);
		}*/
		static unsigned long gen(unsigned long seed, int x, int y, int z){
			return RandomSequence(seed ^ x ^ (z << 16), y ^ ((unsigned long)z >> 16)).next() / (ULONG_MAX / baseMax);
/*			unsigned long idum = seed;
			unsigned long state = 0xdeadbeef;
			for(int i = 0; i < 32; i++){
				idum = 1664525L * idum + 1013904223L;
				state ^= idum & (x << (31 - i) >> 31);
			}
			for(int i = 0; i < 32; i++){
				idum = 1664525L * idum + 1013904223L;
				state ^= idum & (y << (31 - i) >> 31);
			}
			for(int i = 0; i < 32; i++){
				idum = 1664525L * idum + 1013904223L;
				state ^= idum & (z << (31 - i) >> 31);
			}
			return state / (ULONG_MAX / baseMax);*/
/*			unsigned long idum = seed;
			unsigned long state = 0xdeadbeef;
			idum = 1664525L * idum + 1013904223L;
			state ^= 1664525L * (idum ^ (unsigned long)x) + 1013904223L;
			idum = 1664525L * idum + 1013904223L;
			state ^= 1664525L * (idum ^ (unsigned long)y) + 1013904223L;
			idum = 1664525L * idum + 1013904223L;
			state ^= 1664525L * (idum ^ (unsigned long)z) + 1013904223L;
			return state / (ULONG_MAX / baseMax);*/
//			final(x,y,z);
//			z += seed;
//			return (1664525L * z + 1013904223L) / (ULONG_MAX / baseMax);
		}
	};

	double persistence = param.persistence;
	int work2[CELLSIZE][CELLSIZE][CELLSIZE] = {0};
	int octave;
	int xi, yi, zi;

	double factor = 1.0;
	double sumfactor = 0.0;

	// Accumulate signal over octaves to produce Perlin noise.
	for(octave = 0; octave < param.octaves; octave += 1){
		int cell = 1 << octave;
		if(octave == 0){
			for(xi = 0; xi < CELLSIZE; xi++) for(yi = 0; yi < CELLSIZE; yi++) for(zi = 0; zi < CELLSIZE; zi++)
				work2[xi][yi][zi] = Random::gen(param.seed, (xi + param.xofs), (yi + param.yofs), zi + param.zofs);
		}
		else for(xi = 0; xi < CELLSIZE; xi++) for(yi = 0; yi < CELLSIZE; yi++) for(zi = 0; zi < CELLSIZE; zi++){
			int xj, yj, zj;
			double sum = 0;
			int xsm = SignModulo(xi + param.xofs, cell);
			int ysm = SignModulo(yi + param.yofs, cell);
			int zsm = SignModulo(zi + param.zofs, cell);
			int xsd = SignDiv(xi + param.xofs, cell);
			int ysd = SignDiv(yi + param.yofs, cell);
			int zsd = SignDiv(zi + param.zofs, cell);
			for(xj = 0; xj <= 1; xj++){
				int xfactor = xj ? xsm : (cell - xsm - 1);
				if(xfactor == 0) // Skip rest of this iteration if factor is 0
					continue;
				for(yj = 0; yj <= 1; yj++){
					int yfactor = yj ? ysm : (cell - ysm - 1);
					if(yfactor == 0) // Skip rest of this iteration if factor is 0
						continue;
					for(zj = 0; zj <= 1; zj++){
						int zfactor = zj ? zsm : (cell - zsm - 1);
						if(zfactor == 0) // Skip rest of this iteration if factor is 0
							continue;
						sum += (double)(Random::gen(param.seed, (xsd + xj), (ysd + yj), zsd + zj))
						* xfactor / (double)cell
						* yfactor / (double)cell
						* zfactor / (double)cell;
					}
				}
			}
			work2[xi][yi][zi] += (int)(sum * factor);
		}
		sumfactor += factor;
		factor /= param.persistence;
	}

	// Return result
	for(int xi = 0; xi < CELLSIZE; xi++) for(int yi = 0; yi < CELLSIZE; yi++) for(int zi = 0; zi < CELLSIZE; zi++){
		callback(work2[xi][yi][zi], baseMax * sumfactor, xi, yi, zi);
	}
}

}

#endif
