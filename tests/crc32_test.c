/** \file
 * \brief CRC32 reference implementation by Wikipedia with execution performance profiling
 */
#include "clib/crc32.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	if(argc < 2){
		printf("usage: %s file [repeats]\n", argv[0]);
		printf("   Calculates file's CRC32 repeats times. Default once.\n");
		printf("usage: %s -m outfile\n", argv[0]);
		printf("   Outputs crc_table for inclusion\n");
		return 1;
	}
	if(!strcmp(argv[1], "-m")){
		int n;
		const uint32_t *crc_table;
		if(argc < 3){
			printf("Missing outfile\n");
			return 0;
		}
		FILE *fp = fopen(argv[2], "w");
		if(!fp){
			printf("Cannot open outfile\n");
			return 0;
		}
		fprintf(fp, "/* crc32.h -- tables for rapid CRC calculation\n");
		fprintf(fp, " * Generated automatically by crc32.c\n */\n\n");
		fprintf(fp, "static const uint32_t ");
		fprintf(fp, "crc_table[256] =\n{\n");
		crc32_make_crc_table();
		crc_table = crc32_get_crc_table();
		for (n = 0; n < 256; n++)
			fprintf(fp, "%s0x%08lxUL%s", n % 5 ? "" : "    ", crc_table[n],
				n == 255 ? "\n" : (n % 5 == 4 ? ",\n" : ", "));
		fprintf(fp, "};\n");
		fclose(fp);
		return 0;
	}
	{
		clock_t c = clock();
		crc32_make_crc_table();
		printf("maketime %lg\n", (double)(clock() - c) / CLOCKS_PER_SEC);
	}
	{
		FILE *fp = fopen(argv[1], "rb");
		if(!fp){
			printf("cannot open file %s\n", argv[1]);
			return;
		}
		int n = 1, i;
		if(2 < argc){
			n = atoi(argv[2]);
			if(!n)
				return;
		}
		fseek(fp, 0, SEEK_END);
		long sz = ftell(fp);
		printf("File size = %ld\n", sz);
		printf("Total processed size = %lg\n", (double)sz * n);
		fseek(fp, 0, SEEK_SET);
		uint8_t *buf = malloc(sz);
		fread(buf, sz, 1, fp);
/*		for(i = 0; i < sz; i++)
			printf("%02X ", buf[i]);*/
		fclose(fp);
		clock_t c = clock();
		volatile uint32_t crc = 0;
		for(i = 0; i < n; i++){
			crc = crc32_direct(buf, sz);
		}
		double seconds = (double)(clock() - c) / CLOCKS_PER_SEC;
		printf("crctime %lg\n", seconds);
		printf("bytes per second = %lg\n", (double)sz * n / seconds);
		free(buf);
		printf("%08X\n", crc);
	}
	return 0;
}
