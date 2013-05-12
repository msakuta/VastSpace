#ifndef WAVEMIXER_H
#define WAVEMIXER_H

#define WAVEMIX_SAMPLERATE 11025

extern long wave_volume; /* max 255 */

#ifdef _WIN32
#include <windows.h>

HANDLE CreateWaveOutThread(void);
int addsound(const unsigned char *src, size_t size, size_t delay, short vol, short pitch, char pan, unsigned short loops, char priority);
int stopsound(char priority);
void initsound(BYTE src[]);
void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT msg, DWORD ins, DWORD p1, DWORD p2);
DWORD WINAPI WaveOutThread(HWAVEOUT *);

extern double listener_pos[3];
extern double listener_xhat[3]; /* x unit vector because the unit system may vary */
int addsound3d(const unsigned char *src, size_t size, size_t delay, double vol, double attn, const double pos[3], unsigned short pitch, unsigned long srcpitch);
int movesound3d(int sid, const double pos[3]);

#else
typedef void *HWAVEOUT;

#define addsound
#define initsound
#define WaveOutProc(hwo, msg, ins, p1, p2)
#define WaveOutThread(pc)

#endif

#endif
