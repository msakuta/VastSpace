/** \file
 * \brief Definition of CoordSys class's finding logic specific classes and functions.
 */
#ifndef COORDSYS_FIND_H
#define COORDSYS_FIND_H

#include "CoordSys.h"
#include "astro.h"

/// \brief We do not want to recompile everything everytime a single function definition is altered.
///
/// By separating definition of a parameter structure, we get much flexibility and less compile time,
/// but watch out to not using incompletely defined structure.
struct CoordSys::FindParam{
	/// \brief The returned brightness if returnBrightness is set true.
	double brightness;

	/// \brief The threshold at which stars having lower brightness will be truncated from eclipse
	/// and penumbra examination.
	///
	/// Examining too faint stars for eclipse and penumbra may cost CPU time. Setting thresold for
	/// those stars will improve performance if there are many stars.
	///
	/// Setting 0 will disable truncation.
	double threshold;

	/// \brief Whether to check eclipses.
	///
	/// Note that eclipse checking is costly. It has O(n^2) of calculation amount, where n is
	/// the number of involved celestial objects. Be sure to set true to this argument only if necessary.
	bool checkEclipse;

	/// \brief Tell the function to return the brightness in the brightness member variable.
	bool returnBrightness;

	FindParam() : checkEclipse(false), returnBrightness(false), brightness(0), threshold(0){}
};

/// \brief The private version of checkEclipse().
///
/// You can call it from anywhere, but you must provide redundant arguments to get it working properly.
/// CoordSys::findBrightest() makes use of it.
double checkEclipse(Astrobj *illuminator, const CoordSys *retcs, const Vec3d &src, const Vec3d &lightSource, const Vec3d &ray);

/// \brief Checks if a given celestial object can illuminate the given position in the universe.
/// \param illuminator The celestial object in question for light ray check.
/// \param retcs The CoordSys of src.
/// \param src The viewer's position. Ray trace check will be performed between illuminator and src.
/// \returns Ratio at the illuminator's brightness if blocking celestial body does not exist.
///          1 means there's no eclipse, while 0 means total eclipse.
///          It can be somewhere in between if you're in penumbra or antumbra.
inline double checkEclipse(Astrobj *illuminator, const CoordSys *retcs, const Vec3d &src){
	if(illuminator && illuminator->absmag < 30){
		Vec3d lightSource = retcs->tocs(vec3_000, illuminator);
		Vec3d ray = src - lightSource;
		double sd = ray.slen();
		return checkEclipse(illuminator, retcs, src, lightSource, ray);
	}
	return 0.;
}

#endif
