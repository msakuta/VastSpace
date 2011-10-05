#ifndef CLIB_RANDOM_MT19937AR
#define CLIB_RANDOM_MT19937AR
/** \file
 * \brief Header file for Mersenne Twister
 */


void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
unsigned long genrand_int32(void);
long genrand_int31(void);
double genrand_real1(void);
double genrand_real2(void);
double genrand_real3(void);
double genrand_res53(void);


#endif
