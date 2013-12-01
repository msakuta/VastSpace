#ifndef WAVEMIXER_H
#define WAVEMIXER_H
#include "export.h"
#include <stddef.h>

#include <cpplib/vec3.h>
#include <cpplib/quat.h>

#define WAVEMIX_SAMPLERATE 11025

namespace audio{

extern long wave_volume; /* max 255 */

#ifdef _WIN32
#include <windows.h>

EXPORT HANDLE CreateWaveOutThread(void);
EXPORT int addsound(const unsigned char *src, size_t size, size_t delay, short vol, short pitch, char pan, unsigned short loops, char priority);
EXPORT int stopsound(char priority);
EXPORT void initsound(void *src);
EXPORT void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT msg, DWORD ins, DWORD p1, DWORD p2);
EXPORT DWORD WINAPI WaveOutThread(HWAVEOUT *);

/// \brief Sets listener's position and orientation for the sound mixer.
/// \param pos The position of the listener, in world coordinates
/// \param rot The rotation orientation of the listener, in world coordinates
EXPORT void setListener(const Vec3d &pos, const Quatd &rot);

struct SoundSource{
	const unsigned char *src;
	size_t size;
	size_t delay;
	Vec3d pos;
	double vol;
	double attn;
	unsigned short pitch;
	unsigned long srcpitch;
	bool loop;
};

EXPORT int addsound3d(const SoundSource &e);
EXPORT int movesound3d(int sid, const double pos[3]);
EXPORT int stopsound3d(int sid);

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
