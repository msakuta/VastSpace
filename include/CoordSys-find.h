#ifndef COORDSYS_FIND_H
#define COORDSYS_FIND_H

#include "CoordSys.h"

/// \brief We do not want to recompile everything everytime a single function definition is altered.
///
/// By separating definition of a parameter structure, we get much flexibility and less compile time,
/// but watch out to not using incompletely defined structure.
struct CoordSys::FindParam{
	/// \brief The returned brightness if returnBrightness is set true.
	double brightness;

	/// \brief Whether to check eclipses.
	///
	/// Note that eclipse checking is costly. It has O(n^2) of calculation amount, where n is
	/// the number of involved celestial objects. Be sure to set true to this argument only if necessary.
	bool checkEclipse;

	/// \brief Tell the function to return the brightness in the brightness member variable.
	bool returnBrightness;

	FindParam() : checkEclipse(false), returnBrightness(false), brightness(0){}
};

#endif
