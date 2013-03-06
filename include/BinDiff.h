#ifndef BINDIFF_H
#define BINDIFF_H
/** \file
 * \brief Definition of BinDiff class and its companions.
 * \sa BinDiff.cpp
 */
#include <stddef.h>
#include <list>
#include <vector>

/// \brief The type of stream offset values used in Patch and BinDiff.
///
/// It's currently 32bit integer, so you cannot generate patches for streams with size of 4GB+.
typedef unsigned patchsize_t;

/// \brief The type that describes a different part of source and destination streams.
struct Patch{
	patchsize_t start; ///< The source stream offset indicating beginning of altered part.
	patchsize_t size; ///< Gives the size of altered part.
	std::vector<unsigned char> buf; ///< Represents the byte sequence to replace the part.
};

/// \brief A class that creates list of patches from the difference of two byte streams that can be fed to applyPatches().
///
/// This routine is made as a object, so you can progressively give a destination byte stream to generate the patch list,
/// meaning you do not to allocate a complete buffer for destination byte stream at once.
class BinDiff{
	const unsigned char *src; ///< The source stream buffer.
	const unsigned char *cur; ///< Position in the source buffer that is currently matching with the destination.
	patchsize_t size; ///< The size of the source stream buffer.
	const unsigned char *srcdiff; ///< The point which a difference begins in the source buffer.
	std::list<Patch> patches; ///< List of resulting patches.
	bool patchClosed; ///< Flag indicating if the last element in patches is continuing or closed.
public:
	/// The user creates a BinDiff object with the source stream as the parameter.
	BinDiff(const unsigned char *src, patchsize_t size) : src(src), cur(src), size(size), srcdiff(NULL), patchClosed(true){}

	/// Feeds a part of destination stream. The source position is advanced by size.
	void put(const unsigned char *dst, patchsize_t size);

	/// Instruct this BinDiff object that there is no more input.
	/// Necessary to handle the case where the destination stream has less size than source.
	void close();

	/// Returns resulting list of patches.
	const std::list<Patch> &getPatches(){return patches;}
};

/// \brief Applies patches to a mutable byte sequence.
void applyPatches(std::vector<unsigned char> &result, const std::list<Patch> &patches);


#endif
