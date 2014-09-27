/// \file
/// \brief Internal definition for noise functions
#ifndef NOISE_INT_H
#define NOISE_INT_H

namespace noises{
// The permutation table can be shared by multiple noise
// implementations to reduce read-only data memory in the
// process.  It's enclosed by a namespace since its name
// is so generic that collision might happen.
extern const unsigned char perm[];
}
using namespace noises;

#endif
