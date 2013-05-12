/** \file
 * \brief CRC32 C++ version's testing program
 */
#include "cpplib/CRC32.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	if(argc < 2){
		printf("usage: %s file [repeats]\n", argv[0]);
		printf("   Calculates file's CRC32 repeats times. Default once.\n");
		return 1;
	}
	{
		FILE *fp = fopen(argv[1], "rb");
		if(!fp){
			printf("cannot open file %s\n", argv[1]);
			return 1;
		}
		int n = 1, i;
		if(2 < argc){
			n = atoi(argv[2]);
			if(!n)
				return 1;
		}
		fseek(fp, 0, SEEK_END);
		long sz = ftell(fp);
		printf("File size = %ld\n", sz);
		printf("Total processed size = %lg\n", (double)sz * n);
		fseek(fp, 0, SEEK_SET);
		uint8_t *buf = (uint8_t*)malloc(sz);
		fread(buf, sz, 1, fp);
/*		for(i = 0; i < sz; i++)
			printf("%02X ", buf[i]);*/
		fclose(fp);
		clock_t c = clock();
		volatile uint32_t crc = 0;
		for(i = 0; i < n; i++){
			crc = cpplib::crc32(buf, sz);
		}
		double seconds = (double)(clock() - c) / CLOCKS_PER_SEC;
		printf("crctime %lg\n", seconds);
		printf("bytes per second = %lg\n", (double)sz * n / seconds);
		free(buf);
		printf("%08X\n", crc);
	}
	return 0;
}
