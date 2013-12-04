/** \file
 * \brief Private header for 3-D sound mixing engine.
 */
#ifndef AUDIO_WAVESUM_H
#define AUDIO_WAVESUM_H
#include "audio/wavemixer.h"
#ifdef __cplusplus
namespace audio{
#endif

struct sounder{
	int sampleBytes;
	union{
		struct{ const uint8_t *src, *cur; };
		struct{ const int16_t *src16, *cur16; };
	};
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

#ifdef __cplusplus
}
using namespace audio;
#endif
#endif
