#ifndef RANDOM_RSEQ_H
#define RANDOM_RSEQ_H
/** \file rseq.h
 * \brief pseudo-random number sequence generator object
 * which generates exactly the same sequence by the same seed number.
 *
 * This header defines a structure and 2 macros. The structure is
 * struct random_sequence, for storing random sequence status. It is
 * the caller's responsibility to allocate memory space for the structure
 * object, by simply declaring the structure object. The structure is at
 * least of size 64 bits, and period of random sequence is at most 2^64,
 * which is approximately 1.8 x 10^19.
 * The one is init_rseq, for initializing random sequence status object,
 * and the other is rseq, for actually retrieving next number of random
 * sequence.
 * The point of saying these routines are macros are that they may evaluate
 * the arguments more than once (which may cause problems in case such a call
 * like 'rseq(a++)' is invoked), and a pointer to a function cannot be
 * retrieved for them.
 * The function versions of the macros are available, with their name postfixed
 * with a 'f'. If you use them, be sure to compile and link "rseq.c" as well.
 *
 * Semantics
 *
 *   struct random_sequence;
 *   void init_rseq(struct random_sequence *, unsigned long seed);
 *   unsigned long rseq(struct random_sequence *);
 *
 */
#include <limits.h>
#include <stdint.h>

#define RSEQMETHOD 5

#define drseq(rsp) ((double)rseq(rsp) / UINT32_MAX)

// Well, since we know the exact size of uint32_t, we can give
// the maximum value, regardless of implementation.
// g++ won't give us this value, so we define it here as the
// last resort.
#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif


struct random_sequence{
#if RSEQMETHOD==0 || RSEQMETHOD==6
	uint32_t i;
#elif RSEQMETHOD==1
	long *k_ma_end, *pk1, *pk2;
	long k_ma[57];
#elif RSEQMETHOD==2
	uint32_t jsr;
#elif RSEQMETHOD==3
	uint32_t a, b;
#else
	uint32_t z, w;
#endif
};

/* function versions are always declared, but may not be linked */
extern void init_rseqf(struct random_sequence *, uint32_t seed);
extern void init_rseqf_double(struct random_sequence *, double seed);
extern uint32_t rseqf(struct random_sequence *);

#if RSEQMETHOD==6
#define init_rseq(rs,seed) {(rs)->i=seed;}
#define initfull_rseq(rs,seed1,seed2) {(rs)->i=seed1^seed2;}
#define rseq(prs) ((prs)->i=1664525L*(prs)->i + 1013904223L) // Method from Numerical Recipes in C by Knuth.
#elif RSEQMETHOD==7
#define init_rseq(rs,seed) {(rs)->w = (((rs)->z = (seed)) ^ 123459876L) * 123459871L;}
#define initfull_rseq(rs,seed1,seed2) {(rs)->w = (((rs)->z = (seed1)) ^ 123459876L) * 123459871L; (rs)->z += (((rs)->w += (seed2)) ^ 1534241562L) * 123459876L;}
/// Combination of 2 Knuth's method from Numerical Recipes in C.
#define rseq(prs) ((((prs)->z=1664525L*(prs)->z + 1013904223L)>>16)|((((prs)->w=1664525L*(prs)->w + 1013904223L)>>16)<<16))
#elif RSEQMETHOD!=5
#define init_rseq init_rseqf
#define rseq rseqf
#else
#define init_rseq(rs,seed) {(rs)->w = (((rs)->z = (seed)) ^ 123459876L) * 123459871L;}
#define initfull_rseq(rs,seed1,seed2) {(rs)->w = (((rs)->z = (seed1)) ^ 123459876L) * 123459871L; (rs)->z += (((rs)->w += (seed2)) ^ 1534241562L) * 123459876L;}
#define rseq(prs)    ((((prs)->z=36969*((prs)->z&65535)+((prs)->z>>16))<<16)+((prs)->w=18000*((prs)->w&65535)+((prs)->w>>16)))
#endif

#endif /* RANDOM_RSEQ_H */
