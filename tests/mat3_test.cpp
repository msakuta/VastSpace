/** \file
 * \brief CRC32 C++ version's testing program
 */
#include "cpplib/mat3.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static void printmat(const Mat3d &m){
	printf("[%g,%g,%g;\n %g,%g,%g;\n %g,%g,%g]\n", m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
}

int main(int argc, char *argv[]){
	Mat3d test;
	for(int i = 0; i < 9; i++)
		test[i] = (double)rand() / RAND_MAX;

	printf("test = random() =\n");
	printmat(test);

	Mat3d itest = test.inverse();

	printf("itest = test.inverse() =\n");
	printmat(itest);

	Mat3d prod = test * itest;

	printf("prod = test * itest =\n");
	printmat(prod);

	Mat3d iprod = itest * test;

	printf("iprod = itest * test =\n");
	printmat(iprod);

	return 0;
}
