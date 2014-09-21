/** \file
 * \brief CRC32 C++ version's testing program
 */
#include "cpplib/mat3.h"
extern "C"{
#include "clib/timemeas.h"
}
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static void printmat(const Mat3d &m){
	printf("[%g,%g,%g;\n %g,%g,%g;\n %g,%g,%g]\n", m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
}

static void printvec3(const Vec3i &m){
	printf("[%d,%d,%d]\n", m[0], m[1], m[2]);
}

int main(int argc, char *argv[]){

	Mat3i mati(1,2,3,4,5,6,7,8,9);

	// Member access test
	printvec3(mati.vec3(0));
	printvec3(mati.tvec3(0));

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

	timemeas_t tm;
	TimeMeasStart(&tm);

	for(int i = 0; i < 1000000; i++){
		volatile Mat3d itest2 = test.inverse();
	}

	printf("inverse time 100000: %g\n", TimeMeasLap(&tm));

	TimeMeasStart(&tm);

	for(int i = 0; i < 1000000; i++){
		const Mat3d &mat3 = test;
		volatile double det = mat3[0] * mat3[4] * mat3[8] + mat3[1] * mat3[5] * mat3[6] + mat3[2] * mat3[3] * mat3[7]
			- mat3[0] * mat3[5] * mat3[7] - mat3[2] * mat3[4] * mat3[6] - mat3[1] * mat3[3] * mat3[8];
		volatile Mat3d imat3 = Mat3d(
				Vec3d(mat3[4] * mat3[8] - mat3[5] * mat3[7], mat3[7] * mat3[2] - mat3[1] * mat3[8], mat3[1] * mat3[5] - mat3[4] * mat3[2]) / det,
				Vec3d(mat3[6] * mat3[5] - mat3[3] * mat3[8], mat3[0] * mat3[8] - mat3[6] * mat3[2], mat3[3] * mat3[2] - mat3[0] * mat3[5]) / det,
				Vec3d(mat3[3] * mat3[7] - mat3[6] * mat3[4], mat3[6] * mat3[1] - mat3[0] * mat3[7], mat3[0] * mat3[4] - mat3[1] * mat3[3]) / det
				);
	}

	printf("inverse time 100000: %g\n", TimeMeasLap(&tm));

	getchar();

	return 0;
}
