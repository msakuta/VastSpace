#ifndef PERLINNOISE_H
#define PERLINNOISE_H
/** \file
 * \brief Defines Perlin Noise generator classes and functions.
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

namespace PerlinNoise{

/// \brief Callback object that receives result of perlin_noise
///
/// We do this because not all application needs temporary memry to store the result.
class PerlinNoiseCallback{
public:
	virtual void operator()(float, int ix, int iy) = 0;
};

/// \brief Callback object that will assign noise values to float 2-d array field.
template<int CELLSIZE>
class FieldAssign : public PerlinNoiseCallback{
	typedef float (&fieldType)[CELLSIZE][CELLSIZE];
	fieldType field;
public:
	FieldAssign(fieldType field) : field(field){}
	void operator()(float f, int ix, int iy){
		field[ix][iy] = f;
	}
};

/// \brief Parameters given to perlin_noise.
struct PerlinNoiseParams{
	long seed;
	long octaves;
	int xofs, yofs;
	double persistence;

	PerlinNoiseParams(
		long seed = 0,
		double persistence = 0.5,
		long octaves = 4,
		int xofs = 0,
		int yofs = 0)
		: seed(seed),
		persistence(persistence),
		octaves(octaves),
		xofs(xofs),
		yofs(xofs)
	{
	}
};

template<int CELLSIZE>
inline void perlin_noise(long seed, PerlinNoiseCallback &callback, int xofs = 0, int yofs = 0){
	perlin_noise<CELLSIZE>(PerlinNoiseParams(seed, 0.5, xofs, yofs), callback);
}

/// \brief Generate Perlin Noise and return it through callback.
/// \param param Parameters to generate the noise.
/// \param callback Callback object to receive the result.
template<int CELLSIZE>
void perlin_noise(const PerlinNoiseParams &param, PerlinNoiseCallback &callback){
	const int baseMax = 255;

	struct Random{
		RandomSequence rs;
		Random(long seed, long x, long y) : rs(seed ^ x, y){}
		unsigned long next(){
			unsigned long u = rs.next();
			return (u >> 8) & 0xf0 + (u & 0xf);
		}
	};

	double persistence = param.persistence;
	int work2[CELLSIZE][CELLSIZE] = {0};
	int octave;
	int xi, yi;

	double factor = 1.0;
	double sumfactor = 0.0;

	// Accumulate signal over octaves to produce Perlin noise.
	for(octave = 0; octave < param.octaves; octave += 1){
		int cell = 1 << octave;
		if(octave == 0){
			for(xi = 0; xi < CELLSIZE; xi++) for(yi = 0; yi < CELLSIZE; yi++)
				work2[xi][yi] = Random(param.seed, (xi + param.xofs), (yi + param.yofs)).next();
		}
		else for(xi = 0; xi < CELLSIZE; xi++) for(yi = 0; yi < CELLSIZE; yi++){
			int xj, yj;
			double sum = 0;
			int xsm = SignModulo(xi + param.xofs, cell);
			int ysm = SignModulo(yi + param.yofs, cell);
			int xsd = SignDiv(xi + param.xofs, cell);
			int ysd = SignDiv(yi + param.yofs, cell);
			for(xj = 0; xj <= 1; xj++) for(yj = 0; yj <= 1; yj++){
				sum += (double)(Random(param.seed, (xsd + xj), (ysd + yj)).next())
				* (xj ? xsm : (cell - xsm - 1)) / (double)cell
				* (yj ? ysm : (cell - ysm - 1)) / (double)cell;
			}
			work2[xi][yi] += (int)(sum * factor);
		}
		sumfactor += factor;
		factor /= param.persistence;
	}

	// Return result
	for(int xi = 0; xi < CELLSIZE; xi++) for(int yi = 0; yi < CELLSIZE; yi++){
		callback(float((double)work2[xi][yi] / baseMax / sumfactor), xi, yi);
	}
}

}

#endif
