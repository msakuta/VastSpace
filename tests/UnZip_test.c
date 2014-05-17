/** \file
 * \brief UnZip alignment test
 */
#include "../src/UnZip.c"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


#define CHECK(t,a) printf(#t " " #a "; %lu\n", offsetof(ZipLocalHeader, a))
#define CHECKC(t,a) printf(#t " " #a "; %lu\n", offsetof(ZipCentralDirHeader, a))
#define CHECKE(t,a) printf(#t " " #a "; %lu\n", offsetof(ZipEndCentDirHeader, a))

int main(int argc, char *argv[]){
	printf("ZipLocalHeader size: %d\n", sizeof(ZipLocalHeader));
	printf("offsets\n");
	CHECK(uint32_t, signature);
	CHECK(uint16_t, version);
	CHECK(uint16_t, flag);
	CHECK(uint16_t, method);
	CHECK(uint16_t, modtime);
	CHECK(uint16_t, moddate);
	CHECK(uint32_t, crc32);
	CHECK(uint32_t, csize);
	CHECK(uint32_t, usize);
	CHECK(uint16_t, namelen);
	CHECK(uint16_t, extralen);

	printf("\nZipCentralDirHeader size: %d\n", sizeof(ZipCentralDirHeader));
	printf("offsets\n");
	CHECKC(uint32_t, signature);
	CHECKC(uint16_t, madever);
	CHECKC(uint16_t, needver);
	CHECKC(uint16_t, option);
	CHECKC(uint16_t, comptype);
	CHECKC(uint16_t, filetime);
	CHECKC(uint16_t, filedate);
	CHECKC(uint32_t, crc32);
	CHECKC(uint32_t, compsize);
	CHECKC(uint32_t, uncompsize);
	CHECKC(uint16_t, fnamelen);
	CHECKC(uint16_t, extralen);
	CHECKC(uint16_t, commentlen);
	CHECKC(uint16_t, disknum);
	CHECKC(uint16_t, inattr);
	CHECKC(uint32_t, outattr);
	CHECKC(uint32_t, headerpos);

	printf("\nZipEndCentDirHeader size: %d\n", sizeof(ZipEndCentDirHeader));
	printf("offsets\n");
	CHECKE(uint32_t, signature);
	CHECKE(uint16_t, disknum);
	CHECKE(uint16_t, startdisknum);
	CHECKE(uint16_t, diskdirentry);
	CHECKE(uint16_t, direntry);
	CHECKE(uint32_t, dirsize);
	CHECKE(uint32_t, startpos);
	CHECKE(uint16_t, commentlen);

	return 0;
}
