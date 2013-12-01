/** \file
 * \brief Private header for 3-D sound mixing engine.
 */
#ifndef AUDIO_WAVESUM_H
#define AUDIO_WAVESUM_H
#include "audio/wavemixer.h"
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

struct movablesounder8 : SoundSource{
	size_t left;
	short serial;
};


extern void wavesum8s(unsigned short *dst, struct sounder8 src[], unsigned long maxt, int nsrc);
extern void wavesum8sMMX(unsigned short *dst, struct sounder8 src[], unsigned long maxt, int nsrc);
extern void amat4translate(double mr[16], double sx, double sy, double sz);

#ifdef __cplusplus
}
using namespace audio;
#endif
#endif
