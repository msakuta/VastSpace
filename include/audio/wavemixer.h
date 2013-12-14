/** \file
 * \brief Definition of 3-D sound mixing engine details.
 */
#ifndef AUDIO_WAVEMIXER_H
#define AUDIO_WAVEMIXER_H
#include "export.h"
#include <stddef.h>
#include <stdint.h>

#include <cpplib/vec3.h>
#include <cpplib/quat.h>

#define WAVEMIX_SAMPLERATE 11025

namespace audio{

extern long wave_volume; /* max 255 */
extern int g_debug_sound;
extern int g_interpolate_sound;
extern int g_interpolate_sound_3d;

#ifdef _WIN32
#include <windows.h>

EXPORT HANDLE CreateWaveOutThread(void);

struct SoundSourceBase{
	int sampleBytes;
	union{
		const uint8_t *src;
		const int16_t *src16;
	};
	size_t size;
	size_t delay;
	double pitch;
	unsigned long rate; ///< Sampling rate of samples

	unsigned calcPitch()const;
};

struct SoundSource : public SoundSourceBase{
	unsigned vol;
	int pan;
	int loops;
	int priority;
};

EXPORT int addsound(const SoundSource &s);
EXPORT int stopsound(char priority);
EXPORT bool isEndSound(int sid);

/// \brief Sets listener's position and orientation for the sound mixer.
/// \param pos The position of the listener, in world coordinates
/// \param rot The rotation orientation of the listener, in world coordinates
EXPORT void setListener(const Vec3d &pos, const Quatd &rot);

struct SoundSource3D : public SoundSourceBase{
	Vec3d pos;
	double vol;
	double attn;
	bool loop;
};

EXPORT int addsound3d(const SoundSource3D &e);
EXPORT int movesound3d(int sid, const Vec3d &pos);
EXPORT int volumesound3d(int sid, double vol);
EXPORT int pitchsound3d(int sid, double pitch);
EXPORT int stopsound3d(int sid);
EXPORT bool isEndSound3D(int sid);

#else
typedef void *HWAVEOUT;

#define addsound
#define initsound
#define WaveOutProc(hwo, msg, ins, p1, p2)
#define WaveOutThread(pc)

#endif

}
using namespace audio;

#endif
