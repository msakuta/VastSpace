/** \file
 * \brief Definition of CoordSys class's finding logic specific classes and functions.
 */
#ifndef COORDSYS_FIND_H
#define COORDSYS_FIND_H

#include "CoordSys.h"
#include "astro.h"

/// \brief The private version of checkEclipse().
///
/// You can call it from anywhere, but you must provide redundant arguments to get it working properly.
/// CoordSys::findBrightest() makes use of it.
double checkEclipse(const Astrobj *illuminator, const CoordSys *retcs, const Vec3d &src, const Vec3d &lightSource, const Vec3d &ray, const Astrobj **rethit = NULL);

/// \brief Checks if a given celestial object can illuminate the given position in the universe.
/// \param illuminator The celestial object in question for light ray check.
/// \param retcs The CoordSys of src.
/// \param src The viewer's position. Ray trace check will be performed between illuminator and src.
/// \param rethit A pointer to buffer that receives an Astrobj covering the light source from the camera, if any.
/// \returns Ratio at the illuminator's brightness if blocking celestial body does not exist.
///          1 means there's no eclipse, while 0 means total eclipse.
///          It can be somewhere in between if you're in penumbra or antumbra.
inline double checkEclipse(const Astrobj *illuminator, const CoordSys *retcs, const Vec3d &src, const Astrobj **rethit = NULL){
	if(illuminator && illuminator->absmag < 30){
		Vec3d lightSource = retcs->tocs(vec3_000, illuminator);
		Vec3d ray = src - lightSource;
		double sd = ray.slen();
		return checkEclipse(illuminator, retcs, src, lightSource, ray, rethit);
	}
	return 0.;
}

/// \brief The base class for CoordSys::find() callback object.
///
/// We do not want to recompile everything everytime a single function definition is altered.
///
/// By separating definition of a parameter structure, we get much flexibility and less compile time,
/// but watch out to not using incompletely defined structure.
struct EXPORT CoordSys::FindCallback{
	/// \brief The function to be called for each CoordSys it finds.
	/// \return true if continue, otherwise enumeration aborts.
	virtual bool invoke(CoordSys *) = 0;
};

/// \brief The const version of CoordSys::FindCallback.
struct EXPORT CoordSys::FindCallbackConst{
	virtual ~FindCallbackConst();
	/// \brief The function to be called for each CoordSys it finds.
	/// \return true if continue, otherwise enumeration aborts.
	virtual bool invoke(const CoordSys *) = 0;
};


/// \brief The criterion to find the nearest CoordSys to a given point.
struct EXPORT FindNearestCoordSys : CoordSys::FindCallbackConst{
	double best; ///< The score (squared distance).
	const CoordSys *bestcs; ///< The best matched CoordSys so far.
	const CoordSys *srccs; ///< The source CoordSys.
	Vec3d srcpos; ///< The source position.
	FindNearestCoordSys(const CoordSys *cs, const Vec3d &pos) : best(1e25), bestcs(NULL), srccs(cs), srcpos(pos){}
	bool invoke(const CoordSys *cs){
		if(!filter(cs))
			return true;
		double sd = (srccs->tocs(vec3_000, cs) - srcpos).slen();
		if(sd < best){
			best = sd;
			bestcs = cs;
		}
		return true;
	}

	/// \brief The virtual predicate function to discard unwanted objects from find result.
	virtual bool filter(const CoordSys *cs){return true;}
};

/// \brief Finds the nearest Astrobj to a given point.
struct EXPORT FindNearestAstrobj : FindNearestCoordSys{
	FindNearestAstrobj(const CoordSys *cs, const Vec3d &pos) : FindNearestCoordSys(cs, pos){}
	bool filter(const CoordSys *cs){
		return cs->toAstrobj();
	}
	/// \brief Obtains best matched Astrobj.
	const Astrobj *getAstrobj(){return bestcs ? static_cast<const Astrobj*>(bestcs) : NULL;}
};

/// \brief Finds the brightest astronomical object around. Multiple inheritance is no longer necessary.
struct EXPORT FindBrightestAstrobj : CoordSys::FindCallbackConst{
	/// \brief The returned brightness if returnBrightness is set true.
	double brightness;

	/// \brief The brightness would be this value if the light source is not covered by an Astrobj.
	double nonShadowBrightness;

	/// \brief The threshold at which stars having lower brightness will be truncated from eclipse
	/// and penumbra examination.
	///
	/// Examining too faint stars for eclipse and penumbra may cost CPU time. Setting thresold for
	/// those stars will improve performance if there are many stars.
	///
	/// Setting 0 will disable truncation.
	double eclipseThreshold;

	/// \brief The threshold to omit enumerating too faint light sources.
	double threshold;

	/// \brief The returned Astrobj that is casting shadow.
	const Astrobj *eclipseCaster;

	/// \brief Whether to check eclipses.
	///
	/// Note that eclipse checking is costly. It has O(n^2) of calculation amount, where n is
	/// the number of involved celestial objects. Be sure to set true to this argument only if necessary.
	bool checkEclipse;

	/// \brief Tell the function to return the brightness in the brightness member variable.
	bool returnBrightness;

	/// \brief Desired count of Astrobjs to return in a list.  Default is 1.
	int resultCount;

	const CoordSys *retcs; ///< The CoordSys for returned point.
	Vec3d src; ///< The source position from where observing brightness.

	struct ResultSet{
		const Astrobj *cs;
		double brightness;
		Vec3d pos; ///< Position cache in returning CoordSys
	};
	std::vector<ResultSet> results; ///< The returned Astrobj

	/// \brief The constructor with default parameters.
	FindBrightestAstrobj(const CoordSys *retcs, const Vec3d &src, int resultCount = 1) :
		checkEclipse(false), returnBrightness(false), brightness(0),
		eclipseThreshold(0),
		threshold(0),
		eclipseCaster(NULL),
		retcs(retcs), src(src), resultCount(resultCount){}
	bool invoke(const CoordSys *cs);
};

#endif
