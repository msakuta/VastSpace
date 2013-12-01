#ifndef WAVESUM_H
#define WAVESUM_H
#include <stddef.h>
#ifdef __cplusplus
namespace audio{
#endif


struct sounder8{
	const unsigned char *src, *cur;
	size_t left, size;
	size_t delay;
	unsigned short vol;
	short pitch;
	unsigned short loops;
	signed char pan;
	signed char priority;
};

struct movablesounder8{
	const unsigned char *src;
	size_t left;
	size_t delay;
	short serial;
	unsigned short pitch; /* fixed point value of 256 */
	double pos[3];
	double vol;
	double attn;
};


extern void wavesum8s(unsigned short *dst, struct sounder8 src[], unsigned long maxt, int nsrc);
extern void wavesum8sMMX(unsigned short *dst, struct sounder8 src[], unsigned long maxt, int nsrc);
extern void amat4translate(double mr[16], double sx, double sy, double sz);

#ifdef __cplusplus
}
using namespace audio;
#endif
#endif
