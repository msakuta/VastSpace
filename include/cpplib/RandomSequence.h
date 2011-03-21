#ifndef CPPLIB_RANDOMSEQUENCE_H
#define CPPLIB_RANDOMSEQUENCE_H
/* rseq.h
 * 
 * pseudo-random number sequence generator object, ported from clib,
 * which generates exactly the same sequence by the same seed number.
 *
 */
#include <limits.h>
#include <clib/rseq.h>

struct RandomSequence : random_sequence{
	RandomSequence(unsigned long seed1 = 5242314, unsigned long seed2 = 6363893);
	RandomSequence(const void *pointer){init((unsigned long)pointer, 6363893);}
	unsigned long next();
	double nextd(){return ((double)next() / ULONG_MAX);}
	double nextGauss(){return (nextd() - .5) + (nextd() - .5);}
	void init(unsigned long seed1, unsigned long seed2);
};


/* --- Implementations --- */

inline RandomSequence::RandomSequence(unsigned long seed1, unsigned long seed2){
	init(seed1, seed2);
}

inline void RandomSequence::init(unsigned long seed1, unsigned long seed2){
	w = ((z = (seed1)) ^ 123459876L) * 123459871L;
	z += ((w += (seed2)) ^ 1534241562L) * 123459876L;
}

inline unsigned long RandomSequence::next(){
	return (((z=36969*(z&65535)+(z>>16))<<16)+(w=18000*(w&65535)+(w>>16)));
}

#endif /* CPPLIB_RANDOMSEQUENCE_H */
