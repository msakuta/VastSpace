#include "clib/rseq.h"

#define MBIG  ULONG_MAX
#define MASK  23459876L
#define MSEED 61803398

typedef struct random_sequence rst;


void init_rseqf(rst *rs, unsigned long seed){
#if RSEQMETHOD==0
	rs->i = seed ^ 123459876L;
#elif RSEQMETHOD==1
	long mj, mk;
	int i, k;

	rs->k_ma_end = &rs->k_ma[56];
	mj = (MSEED - (seed ^ MASK)) % MBIG;
	rs->k_ma[55] = mj;
	mk = 1L;
	for(i = 1; i <= 54; i++)
	{
		k = (21 * i) % 55;
		rs->k_ma[k] = mk;
		mk = mj - mk;
		if(mk < 0)	mk += MBIG;
		mj = rs->k_ma[k];
	}
	for(k = 1; k <= 4; k++)
	{
		for(i = 1; i <= 55; i++)
		{
			rs->k_ma[i] -= rs->k_ma[1 + (i + 30) % 55];
			if(rs->k_ma[i] < 0)	rs->k_ma[i] += MBIG;
		}
	}
	rs->pk1 = &rs->k_ma[0];
	rs->pk2 = &rs->k_ma[31];
#elif RSEQMETHOD==2
	rs->jsr = seed;
#elif RSEQMETHOD==3
	rs->a = rs->b = seed;
#else
/*	rs->z = (((rs->w = seed) ^ MASK) + 8651) * 123459871L;*/
	rs->w = (((rs->z = seed) ^ MASK) /* + 8651*/) * 123459871L;
#endif
}

// Implementation for machine with sizeof(unsigned long) == 4 and sizeof(double) == 8.
void init_rseqf_double(rst *rs, double seed){
	initfull_rseq(rs, *(unsigned long*)&seed, ((unsigned long*)&seed)[1]);
}


#define SHR3(jsr)  (jsr^=(jsr<<17), jsr^=(jsr>>13), jsr^=(jsr<<5))
#define FIB(a,b)   ((b=a+b),(a=b-a))
#define znew(z)   (z=36969*(z&65535)+(z>>16))
#define wnew(w)   (w=18000*(w&65535)+(w>>16))
#define MWC(z,w)    ((znew(z)<<16)+wnew(w) )
unsigned long rseqf(rst *rs)
{
#if RSEQMETHOD==0
	return ((rs->i = (rs->i) * 2110005341UL + 2531011UL) >> 3) * 445223UL;
#elif RSEQMETHOD==1
/*	double d = 1. / (double)MBIG;*/
	long mj;

	if(++rs->pk1 == rs->k_ma_end)	rs->pk1 = &rs->k_ma[1];
	if(++rs->pk2 == rs->k_ma_end)	rs->pk2 = &rs->k_ma[1];
	if((mj = *rs->pk1 - *rs->pk2) < 0)	mj += MBIG;
	return (*rs->pk1 = mj);
#elif RSEQMETHOD==2
	return SHR3((rs->jsr));
#elif RSEQMETHOD==3
	return FIB((rs->a),(rs->b));
#else
	return MWC((rs->z),(rs->w));
#endif
}

/*	double d = 1. / 4294967296.;*/
