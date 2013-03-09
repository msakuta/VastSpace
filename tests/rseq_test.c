/** \file
 * \brief CRC32 reference implementation by Wikipedia with execution performance profiling
 */
#include "clib/rseq.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	if(argc < 2){
		printf("usage: %s repeats\n", argv[0]);
		printf("   Calculates file's CRC32 repeats times. Default once.\n");
		return 1;
	}
	{
		struct random_sequence rs;
		double i, n = atof(argv[1]);
		uint32_t inival;
		clock_t c = clock();
		volatile uint32_t crc = 0;
		init_rseq(&rs, 1);
		inival = rseq(&rs);
		for(i = 0; i < n; i++){
			crc = rseq(&rs);
			if(inival == crc){
				printf("Frequency %lg\n", i);
				break;
			}
		}
		double seconds = (double)(clock() - c) / CLOCKS_PER_SEC;
		printf("second = %lg\n", seconds);
		printf("generation per second = %lg\n", i / seconds);
	}
	return 0;
}
