/** \file
 * \brief Private header for 3-D sound mixing engine.
 */
#ifndef AUDIO_WAVESUM_H
#define AUDIO_WAVESUM_H
#include "audio/wavemixer.h"
#ifdef __cplusplus
namespace audio{
#endif

struct sounder : SoundSource{
	union{
		const uint8_t *cur;
		const int16_t *cur16;
	};
	size_t left;

	sounder(){}
	sounder(const SoundSource &s) : SoundSource(s), cur(src), left(size){}
};

struct sounder3d : SoundSource3D{
	size_t left;
	short serial;

	sounder3d(){}
	sounder3d(const SoundSource3D &s);
};

#ifdef __cplusplus
}
using namespace audio;
#endif
#endif
