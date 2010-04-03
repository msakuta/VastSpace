#ifndef RANDOM_RSEQ_H
#define RANDOM_RSEQ_H
/* rseq.h
 * pseudo-random number sequence generator object
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

#define RSEQMETHOD 5

#define drseq(rsp) ((double)rseq(rsp) / ULONG_MAX)


struct random_sequence{
#if RSEQMETHOD==0
	unsigned long i;
#elif RSEQMETHOD==1
	long *k_ma_end, *pk1, *pk2;
	long k_ma[57];
#elif RSEQMETHOD==2
	unsigned long jsr;
#elif RSEQMETHOD==3
	unsigned long a, b;
#else
	unsigned long z, w;
#endif
};

/* function versions are always declared, but may not be linked */
extern void init_rseqf(struct random_sequence *, unsigned long seed);
extern void init_rseqf_double(struct random_sequence *, double seed);
extern unsigned long rseqf(struct random_sequence *);

#if RSEQMETHOD!=5
#define init_rseq init_rseqf
#define rseq rseqf
#else
#define init_rseq(rs,seed) {(rs)->w = (((rs)->z = (seed)) ^ 123459876L) * 123459871L;}
#define initfull_rseq(rs,seed1,seed2) {(rs)->w = (((rs)->z = (seed1)) ^ 123459876L) * 123459871L; (rs)->z += (((rs)->w += (seed2)) ^ 1534241562L) * 123459876L;}
#define rseq(prs)    ((((prs)->z=36969*((prs)->z&65535)+((prs)->z>>16))<<16)+((prs)->w=18000*((prs)->w&65535)+((prs)->w>>16)))
#endif

#endif /* RANDOM_RSEQ_H */
