#include "draw/blackbody.h"
#include <clib/c.h>


/// Pseudo RGB colors of black body emission, computed by Planck's law.
static const Vec3f planckDistrib[] = {
	Vec3f(11.102f, 3.2267f, 1.3630f), // 3000K
	Vec3f(187.15f, 118.92f, 82.703f), // 5000K
	Vec3f(646.86f, 566.33f, 485.00f), // 7000K
	Vec3f(1328.7f, 1375.5f, 1315.6f), // 9000K
	Vec3f(2155.8f, 2467.9f, 2523.5f), // 11000K
	Vec3f(3077.1f, 3763.3f, 4021.3f), // 13000K
};

/// Linearly interpolate sampled Planck distribution to compute RGB color in given temperature.
///
/// To minimize round errors, intermediate calculatoins are performed in double precision.
Vec3f blackbodyEmit(double temperature){
	const double normalizer = 1. / 4000.;
	if(temperature < 3000)
		return ((Vec3d(0,0,0) * (3000. - temperature) + planckDistrib[0].cast<double>() * (temperature)) / 3000. * normalizer).cast<float>();
	int i = int(floor((temperature - 3000.) / 2000.));
	double f = (temperature - 3000) / 2000. - i;
	if(0 <= i && i + 1 < numof(planckDistrib))
		return ((planckDistrib[i].cast<double>() * (1. - f) + planckDistrib[i+1].cast<double>() * f) * normalizer).cast<float>();
	else
		return planckDistrib[numof(planckDistrib)-1] * float(normalizer);
}
