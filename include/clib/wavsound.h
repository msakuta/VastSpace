#ifndef CLIB_WAVSOUND_H
#define CLIB_WAVSOUND_H

#ifdef _WIN32
#include <windows.h>
extern HINSTANCE g_hInstance;
#endif

extern double wave_sonic_speed;

extern struct wavecache *cacheWaveFile(const char *lpszWAVEFileName);
extern int playWAVEFile(const char *lpszWAVEFileName);
extern int playWaveCustom(const char *lpszWAVEFileName, unsigned short vol, unsigned short pitch, signed char pan, unsigned short loops, signed char priority, double delay);
extern int playWave3D(const char *lpszWAVEFileName, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay);
extern int playWave3DPitch(const char *lpszWAVEFileName, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay, unsigned short pitch);
extern int playMemoryWave3D(const unsigned char *, size_t size, short pitch, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay);

#endif
