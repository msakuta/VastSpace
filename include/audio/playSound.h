/** \file
 * \brief Definition of 3-D sound mixing engine file loading interface.
 */
#ifndef AUDIO_PLAYSOUND_H
#define AUDIO_PLAYSOUND_H

#include "export.h"
#include <cpplib/vec3.h>
#ifdef _WIN32
#include <windows.h>
extern HINSTANCE g_hInstance;
#endif

namespace audio{

extern double wave_sonic_speed;

extern EXPORT struct wavecache *cacheWaveFile(const char *lpszWAVEFileName);
extern EXPORT int playWAVEFile(const char *lpszWAVEFileName);
extern EXPORT int playWaveCustom(const char *lpszWAVEFileName, size_t delay, unsigned short vol, unsigned short pitch, signed char pan, unsigned short loops, char priority);

/// \brief Play WAV format sound file in 3-D space.
/// \param lpszWAVEFileName The file name of the WAV.
/// \param src The initial position of the sound in 3-D space.  You can update the position by movesound3d().
/// \param vol Volume of the sound in range [0,1]
/// \param attn Attenuation factor in 3-D space.  The sound reaches further when this value is higher.
/// \param delay Time delay before the sound is actually played back.  Used to simulate speed of sound delay.
/// \param loop If this sound loops forever.  You are responsible to stop the sound when finished.
/// \param pitch Sound pitch factor.  Default (as in file) is 256.  Pitch gets higher if this value is lower than 256.
/// \returns Sound ID used to move or stop after this function returns.
extern EXPORT int playWave3D(const char *lpszWAVEFileName, const Vec3d &src, double vol, double attn, double delay = 0., bool loop = false, unsigned short pitch = 256);

extern EXPORT int playMemoryWave3D(const unsigned char *, size_t size, short pitch, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay);

}
using namespace audio;

#endif
