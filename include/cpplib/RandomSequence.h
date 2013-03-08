#ifndef CPPLIB_RANDOMSEQUENCE_H
#define CPPLIB_RANDOMSEQUENCE_H
/* rseq.h
 * 
 * pseudo-random number sequence generator object, ported from clib,
 * which generates exactly the same sequence by the same seed number.
 *
 */
#include <clib/rseq.h>

struct RandomSequence : random_sequence{
	RandomSequence(uint32_t seed1 = 5242314, uint32_t seed2 = 6363893);
	RandomSequence(const void *pointer);
	unsigned long next();
	double nextd(){return ((double)next() / UINT32_MAX);}
	double nextGauss(){return (nextd() - .5) + (nextd() - .5);}
	void init(uint32_t seed1, uint32_t seed2);
};


/* --- Implementations --- */

inline RandomSequence::RandomSequence(uint32_t seed1, uint32_t seed2){
	init(seed1, seed2);
}

/// \brief Constructs a RandomSequence with a pointer.
///
/// Note that using this constructor is not portable between machines.
inline RandomSequence::RandomSequence(const void *pointer){
//#if UINTPTR_MAX <= UINT32_MAX
	// g++ 4.6.3 doesn't recognize the above macro constants even if we include
	// stdint.h.  So we cannot reliably switch implementation between
	// architectures.  Instead, we use regular if statements and hope that
	// the optimizer do the job.
	if(sizeof(pointer) == sizeof(uint32_t))
		init(*(const uint32_t*)(&pointer), 6363893);
	else if(sizeof(pointer) == 2 * sizeof(uint32_t))
//#else // Assume a pointer has a sizeof of twice as large as int32.
		init(*((const uint32_t*)&pointer), ((const uint32_t*)&pointer)[1]);
//#endif
	else{
		uint32_t seed = 0;
		for(int i = 0; i < sizeof(pointer); i++)
			seed ^= ((uint8_t*)&pointer)[i];
		init(seed, 34135138);
	}
}

inline void RandomSequence::init(uint32_t seed1, uint32_t seed2){
	w = ((z = (seed1)) ^ 123459876L) * 123459871L;
	z += ((w += (seed2)) ^ 1534241562L) * 123459876L;
}

inline unsigned long RandomSequence::next(){
	return (((z=36969*(z&65535)+(z>>16))<<16)+(w=18000*(w&65535)+(w>>16)));
}

#endif /* CPPLIB_RANDOMSEQUENCE_H */
