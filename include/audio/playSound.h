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

/// \brief Play WAV or Ogg Vorbis format sound file.
/// \param fileName The file name of the WAV or Ogg Vorbis file.
/// \param delay Time delay before the sound is actually played back.
/// \param vol Volume of the sound in range [0,256]
/// \param pitch Sound pitch factor.  Default (as in file) is 0.  Pitch gets higher if this value is higher than 0.
/// \param pan Pan position of the sound.
/// \param loops Number of loops.
/// \param priority Priority of the sound.  If added sound have higher priority and no channel is available, existing sound can be stopped.
/// \returns Sound ID used to move or stop after this function returns.
extern EXPORT int playSound(const char *fileName, size_t delay = 0, unsigned short vol = 256, unsigned short pitch = 0, signed char pan = 0, unsigned short loops = 0, char priority = 0);

/// \brief Play WAV or Ogg Vorbis format sound file in 3-D space.
/// \param fileName The file name of the WAV or Ogg Vorbis file.
/// \param src The initial position of the sound in 3-D space.  You can update the position by movesound3d().
/// \param vol Volume of the sound in range [0,1]
/// \param attn Attenuation factor in 3-D space.  The sound reaches further when this value is higher.
/// \param delay Time delay before the sound is actually played back.  Used to simulate speed of sound delay.
/// \param loop If this sound loops forever.  You are responsible to stop the sound when finished.
/// \param pitch Sound pitch factor.  Default (as in file) is 256.  Pitch gets higher if this value is lower than 256.
/// \returns Sound ID used to move or stop after this function returns.
extern EXPORT int playSound3D(const char *fileName, const Vec3d &src, double vol, double attn, double delay = 0., bool loop = false, unsigned short pitch = 256);

/// \brief Play raw PCM data in memory.
extern EXPORT int playMemoryWave3D(const unsigned char *, size_t size, short pitch, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay);

}
using namespace audio;

#endif
