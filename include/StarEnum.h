/** \file StarEnum.h
 * \brief Definition of StarEnum class
 */
#ifndef STARENUM_H
#define STARENUM_H

/// \brief A data structure to store cached data about a randomly generated star.
///
/// Randomly generated stars conventionally doesn't need extra memory to store, but now we
/// want to remember the name and possibly materialized solar system.
struct StarCache{
	gltestp::dstring name;
	WeakPtr<CoordSys> system;
	StarCache(const StarCache &o) : name(o.name), system(o.system){}
	StarCache(gltestp::dstring name = "") : name(name){}
};
/// StarCache is a bit costly data structor for copying, so we use std::list here.
typedef std::list<StarCache> StarCacheList;
typedef std::map<std::tuple<int,int,int>, StarCacheList > StarCacheDB;


/// \brief A class to enumerate randomly generated stars.
///
/// This routine had been embedded in drawstarback(), but it may be used by some other routines.
class StarEnum{
public:
	/// \brief The constructor.
	/// \param originPos The center of star generation.  This is usually the player's viewpoint.
	/// \param numSectors Count of sectors to generate around the center.
	///        Higher value generates more stars at once, but it may cost more computing time.
	/// \param genCache Generate StarCache object for each enumerated star.
	StarEnum(const Vec3d &originPos, int numSectors, bool genCache = false);

	/// \brief Retrieves next star in this star generation sequence.
	/// \param pos Position of generated star.
	/// \param cache Pointer to a buffer to receive a pointer to the StarCache object associated with
	///        the enumerated star.
	bool next(Vec3d &pos, StarCache **cache = NULL);

	/// \brief Returns the random sequence for current star.
	RandomSequence *getRseq();

	/// Size of stellar sectors
	static const double sectorSize;

	/// Cached star catalog
	static StarCacheDB starCacheDB;

protected:
	Vec3d plpos;
	Vec3i cen;
	RandomSequence rs; ///< Per-sector random number sequence
	RandomSequence rsStar; ///< Per-star random number sequence
	int numSectors; ///< Size of cube measured in sectors to enumerate
	int gx, gy, gz;
	int numstars;
	StarCacheList *cacheList;
	StarCacheList::iterator it, itend;
	bool genCache;
	bool newCell();
};

#endif /* STARENUM_H_ */
