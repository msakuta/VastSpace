/** \file
 * \brief random_sequence tests and Mersenne Twister tests program
 */
#include "clib/rseq.h"
#include "clib/random/mt19937ar.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	double n = 1;
	int test_init = 0;
	int test_method = 0;
	int test_period = 0;
	if(argc < 2){
		printf("usage: %s [-i] [-m] [-p] repeats\n", argv[0]);
		printf("   Tests pseudo random number generator algorithms repeats times. Default once.\n");
		printf("   -i Profiles initialization cost instead of generation.\n");
		printf("   -m Profiles Mersenne Twister.\n");
		printf("   -p Profiles period of the number sequence.\n");
		return 1;
	}
	{
		int a = 0;
		for(a = 1; a < argc; a++){
			if(!strcmp(argv[a], "-i")){
				test_init = 1;
			}
			else if(!strcmp(argv[a], "-m"))
				test_method = 1; // Test Mersenne Twister
			else if(!strcmp(argv[a], "-p"))
				test_period = 1;
			else
				n = atof(argv[a]);
		}
	}
	if(test_init){
		struct random_sequence rs;
		double i;
		clock_t c = clock();
		if(test_method == 0){
			volatile uint32_t c = 0;
			for(i = 0; i < n; i++){
				init_rseq(&rs, i);
				c = rseq(&rs); // Make sure initialization runs by taking side effects
			}
		}
		else{
			volatile uint32_t crc = 0;
			for(i = 0; i < n; i++){
				init_genrand(i);
				crc = genrand_int32(); // Make sure initialization runs by taking side effects
			}
		}
		double seconds = (double)(clock() - c) / CLOCKS_PER_SEC;
		printf("second = %lg\n", seconds);
		printf("initialization per second = %lg\n", i / seconds);
	}
	else{
		struct random_sequence rs;
		double i;
		clock_t c = clock();
		if(test_method == 0){
			struct random_sequence inirs; // Buffer to hold initial state vector.
			volatile uint32_t v = 0;
			init_rseq(&rs, 1);
			if(!test_period){
				for(i = 0; i < n; i++)
					v = rseq(&rs);
			}
			else{
				inirs = rs;
				for(i = 0; i < n; i++){
					v = rseq(&rs);
					// Detect period by comparing state vector, not by returned value.
					// Note that random_sequence structure could have padding bytes that
					// don't contribute to period, but it's unlikely that those paddings
					// are altered between calls.
					if(!memcmp(&inirs, &rs, sizeof rs)){
						printf("Period %lg\n", i);
						break;
					}
				}
			}
		}
		else{
			volatile uint32_t v = 0;
			init_genrand(1);
			for(i = 0; i < n; i++){
				v = genrand_int32();
				// Measuring period of Mersenne Twister is meaningless, because it
				// has far more internal state vectors than the value returned.
				// We should be taking more than universe's lifetime to reach the period.
/*				if(inival == v){
					printf("period %lg\n", i);
					break;
				}*/
			}
		}
		double seconds = (double)(clock() - c) / CLOCKS_PER_SEC;
		printf("second = %lg\n", seconds);
		printf("generation per second = %lg\n", i / seconds);
	}
	return 0;
}
