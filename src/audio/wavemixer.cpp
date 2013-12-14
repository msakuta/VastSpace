#include "audio/wavemixer.h"
extern "C"{
#include "clib/timemeas.h"
#include "clib/rseq.h"
#include "clib/avec3.h"
}
#include <mmsystem.h>
#include <assert.h>

#define numof(a) (sizeof(a)/sizeof*(a))

#define SAMPLERATE 22050
#define PITCHDIV 256
#define SAMPLEBITS 16
#define MAXMIX 32
#define EPSILON 0.001
#define DEBUG_SOUND_WINDOW 1
#define INTERPOLATE_SOUND 1
#define INTERPOLATE_SOUND_3D 1

namespace audio{

static HWAVEOUT hwo;
static DWORD WINAPI WaveOutThread(HWAVEOUT *phwo);
static void initsound(void *pv);

#if SAMPLEBITS == 8
typedef BYTE sbits;
#elif SAMPLEBITS == 16
typedef int16_t sbits;
#else
#error SAMPLEBITS neither 8 nor 16!
#endif

long wave_volume = 255;
static volatile long wave_current_volume = 255;

// The interpolation switch variables are frequently read by dedicated sound mixing thread,
// so one may concern about synchronization, but they will never be written by
// the sound thread, so the main thread only need to care about writing atomically.
// Default is to interpolate for sound quality.
int g_interpolate_sound = 1;
int g_interpolate_sound_3d = 1;

static sbits soundbuf[2][2048][2];
static WAVEHDR whs[2];
timemeas_t tmwo;
LONG listener_inwater = 0;
static CRITICAL_SECTION gcs;
static HWND hWndWave = NULL;


struct sounder : SoundSource{
	union{
		const uint8_t *cur;
		const int16_t *cur16;
	};
	size_t left;
	unsigned ipitch; ///< Integral pitch multiplier

	sounder(){}
	sounder(const SoundSource &s) : SoundSource(s), cur(src), left(size), ipitch(calcPitch()){}
};

struct sounder3d : SoundSource3D{
	size_t left;
	unsigned ipitch; ///< Integral pitch multiplier
	short serial;

	sounder3d(){}
	sounder3d(const SoundSource3D &s);
};



HANDLE CreateWaveOutThread(void){
#if DEBUG_SOUND_WINDOW
	// We must create the window in the main thread, not the wave mixing thread,
	// because the window's messages are processed in the main thread.
	ATOM atom;
	if(g_debug_sound){
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= (WNDPROC)DefWindowProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= GetModuleHandle(NULL);
		wcex.hIcon			= LoadIcon(wcex.hInstance, IDI_APPLICATION);
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= NULL;
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= "WavePreviewWnd";
		wcex.hIconSm		= LoadIcon(wcex.hInstance, IDI_APPLICATION);

		atom = RegisterClassEx(&wcex);

		RECT rc = {100, 100, 100 + 640, 100 + 480};
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CAPTION, FALSE);

		hWndWave = CreateWindow(LPCTSTR(atom), "WavePreviewWnd", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, GetModuleHandle(NULL), NULL);
	}
#endif

		WAVEFORMATEX wf = {
			WAVE_FORMAT_PCM, //WORD  wFormatTag; 
			2, //WORD  nChannels; 
			SAMPLERATE, //DWORD nSamplesPerSec; 
			SAMPLERATE * 2 * SAMPLEBITS / 8, //DWORD nAvgBytesPerSec; 
			2 * SAMPLEBITS / 8, //WORD  nBlockAlign; 
			SAMPLEBITS, //WORD  wBitsPerSample; 
			0, //WORD  cbSize; 
		};
		MMRESULT mmr;
		DWORD dw;
		HANDLE ret;
//		return NULL;
/*		BYTE buffer[128];
		int i;
		for(i = 0; i < numof(buffer); i++)
			buffer[i] = i < numof(buffer) / 2 ? 255 : 0;
//			buffer[i] = i * 4 <= 255 ? i * 4 : 255 - i * 4 % 256; 
		WAVEHDR wh;*/
		InitializeCriticalSection(&gcs);
		ret = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WaveOutThread, &hwo, 0, &dw);
		mmr = waveOutOpen(&hwo, WAVE_MAPPER, &wf, dw, (DWORD)&hwo, CALLBACK_THREAD);
//		waveOutOpen(&hwo, WAVE_MAPPER, &wf, (DWORD)WaveOutPoster, (DWORD)pc, CALLBACK_FUNCTION);
//		waveOutOpen(&hwo, WAVE_MAPPER, &wf, (DWORD)pc->w, (DWORD)pc, CALLBACK_WINDOW);
/*		wh.lpData = (LPSTR)buffer;
		wh.dwBufferLength = sizeof buffer;
		wh.dwFlags = 0;
		waveOutPrepareHeader(hwo, &wh, sizeof(WAVEHDR));
		wh.dwFlags |= WHDR_BEGINLOOP | WHDR_ENDLOOP;
		wh.dwLoops = 100;
		waveOutWrite(hwo, &wh, sizeof(WAVEHDR));*/
		if(mmr == MMSYSERR_NOERROR){
			int k;
			initsound(soundbuf[0]);
			initsound(soundbuf[1]);
			TimeMeasStart(&tmwo);
	//		memset(soundbuf, 0x80, sizeof soundbuf);
			for(k = 0; k < 2; k++){
				whs[k].lpData = (LPSTR)soundbuf[k];
				whs[k].dwBufferLength = sizeof soundbuf[k];
				whs[k].dwFlags = 0;
				waveOutPrepareHeader(hwo, &whs[k], sizeof(WAVEHDR));
				whs[k].dwFlags |= WHDR_BEGINLOOP | WHDR_ENDLOOP;
				whs[k].dwLoops = 1;
			}
			waveOutWrite(hwo, &whs[0], sizeof(WAVEHDR));
			waveOutWrite(hwo, &whs[1], sizeof(WAVEHDR));
		}
		else
			hwo = NULL;

		return ret;
}

unsigned SoundSourceBase::calcPitch()const{
	return (unsigned)(pitch * rate * PITCHDIV / SAMPLERATE);
}

static sounder sounders[32];

static sounder3d msounders[128];
static short imsounder = 0;

int addsound(const SoundSource &s){
	int i;
	if(s.vol < 16 || !s.size)
		return -1;
	assert(0 < s.pitch);
	for(i = 0; i < numof(sounders); i++){
		sounder &ss = sounders[i];
		if(ss.left == 0 || ss.priority < s.priority
			|| ss.priority == s.priority && (long)s.delay - SAMPLERATE / s.vol < (long)ss.delay - SAMPLERATE / ss.vol)
		{
			ss = sounder(s);
			return i;
		}
	}
	return -1;
}

int stopsound(char priority){
	int i;
	sounder *ps;
	for(i = 0; i < numof(sounders); i++){
		ps = &sounders[i];
		if(ps->priority == priority){
			ps->left = 0;
			ps->loops = 0;
		}
	}
	return i;
}

bool isEndSound(int sid){
	assert(0 <= sid && sid < numof(sounders));
	sounder &s = sounders[sid];
	return s.src == NULL || 0 == s.loops && 0 == s.left;
}

int maddsound(const unsigned char *src, size_t size, size_t delay, const double pos[3], double vol, double attn){
	sounder3d *s;
	s = &msounders[imsounder];
	s->src = (decltype(s->src))src;
	s->left = size;
	s->delay = delay;
	s->serial++;
	VECCPY(s->pos, pos);
	s->vol = vol;
	s->attn = attn;
	imsounder = (imsounder + 1) % numof(msounders);
	return imsounder & (s->serial << 16); /* encode the pointer that it includes the serial number. */
}

static void initsound(void *pv){
	BYTE (*src)[2] = (BYTE(*)[2])pv;
#if 0
	int i;
	for(i = 0; i < numof(soundbuf[0]); i++){
		src[i][0] = 256 * i / numof(soundbuf[0]);
		src[i][1] = 256 - 256 * i / numof(soundbuf[0]);
	}
#elif !defined NDEBUG && 0
	int i;
	struct random_sequence rs;
	init_rseq(&rs, src);
	for(i = 0; i < sizeof soundbuf[0]; i++)
		src[i] = 0x80 + rseq(&rs) % 10 - 5;
#else
#	if SAMPLEBITS == 8
	memset(src, 0x80, sizeof soundbuf[0]);
#	else
	memset(src, 0, sizeof soundbuf[0]);
#	endif
#endif
}

static void wavesum(sbits *dst, sounder src[], unsigned long maxt, int nsrc){
	const volatile int interp = g_interpolate_sound;
	int t;
	for(t = 0; t < maxt; t++){
		long tt0 = 0, tt1 = 0;
		int i;
		for(i = 0; i < nsrc; i++){
			sounder &s = src[i];
			unsigned ipitch = s.ipitch ? s.ipitch : PITCHDIV;
			long lsrct = (t - s.delay) * ipitch;
#if INTERPOLATE_SOUND
			long modt = lsrct % PITCHDIV;
#endif
			long srct = lsrct / PITCHDIV;
#if INTERPOLATE_SOUND
			// If we have a nonzero modulo, the buffer must have one extra sample to interpolate with.
			if(s.delay < srct && srct < s.left + s.delay + (interp && modt))
#else
			if(s.delay < srct && srct < s.left + s.delay)
#endif
			{
				if(s.sampleBytes == 1){
					long value =
#if INTERPOLATE_SOUND
						// The latter half of the conditional (modt != 0) may make the calculation slower due to
						// CPU pipeline invalidation, but is necessary to prevent excess memory access
						interp && modt != 0 ?
							((long)s.cur[srct] - 128) * (PITCHDIV - modt) + ((long)s.cur[srct+1] - 128) * modt / PITCHDIV :
#endif
							((long)s.cur[srct] - 128) * s.vol;
					tt0 += value * (128 + s.pan);
					tt1 += value * (127 - s.pan);
				}
				else{
					long value =
#if INTERPOLATE_SOUND
						interp && modt != 0 ?
							(s.cur16[srct] * (PITCHDIV - modt) + s.cur16[srct+1] * modt) / PITCHDIV * s.vol :
#endif
							s.cur16[srct] * s.vol;
					tt0 += value * (128 + s.pan) / 256;
					tt1 += value * (127 - s.pan) / 256;
				}
			}
		}
#if SAMPLEBITS == 8
		tt0 = tt0 * wave_current_volume / 256 / (256 * 256);
		tt1 = tt1 * wave_current_volume / 256 / (256 * 256);
		dst[t] = (tt0 > 127 ? 255 : tt0 < -128 ? 0 : (BYTE)(tt0 + 128)) |
			((tt1 > 127 ? 255 : tt1 < -128 ? 0 : (BYTE)(tt1 + 128)) << 8);
#else
		tt0 = tt0 * wave_current_volume / 256 / 256;
		tt1 = tt1 * wave_current_volume / 256 / 256;
		dst[t * 2] = (tt0 > SHRT_MAX ? SHRT_MAX : tt0 < SHRT_MIN ? SHRT_MIN : (tt0));
		dst[t * 2 + 1] = (tt1 > SHRT_MAX ? SHRT_MAX : tt1 < SHRT_MIN ? SHRT_MIN : (tt1));
#endif
	}
}

static void wavesumm8s(sbits *dst, sounder3d src[], unsigned long maxt, int nsrc, const Vec3d &view, const Vec3d &xh){
	sounder3d *pms[MAXMIX]; // Temprary msounder array for sorting by calculated volume
	int m = 0;
	const long voldiv = 0x10000; // Volume divisor, adjust by sample bit depth
	long pan[MAXMIX]; // Calculated pan position
	long vol[MAXMIX]; // Calculated volume
	for(int i = 0; i < nsrc; i++) if(src[i].left && src[i].delay < numof(soundbuf[0])){
		double sdist = (src[i].pos - view).slen();

		/* calculate the volume once in a timeslice */
		long v = voldiv * src[i].vol / (1. + sdist / src[i].attn);

		/* dont calculate too small sounds */
		if(v <= 2)
			continue;

		if(m < MAXMIX){
			int j;
			for(j = 0; j < m; j++) if(vol[j] < v)
				break;
			memmove(&pms[j+1], &pms[j], (m - j) * sizeof *pms);
			memmove(&vol[j+1], &vol[j], (m - j) * sizeof *vol);
			pms[j] = &src[i];
			vol[j] = v;
			m++;
		}
		else if(vol[MAXMIX-1] < v){
			/* note that older msounder precedes, to prevent sound chops
			  caused by rapid repeats of sounds having the same volume. */
			for(int j = m-1; j; j--) if(j == 0 || v <= vol[j-1]){
				memmove(&pms[j+1], &pms[j], (m - j - 1) * sizeof *pms);
				memmove(&vol[j+1], &vol[j], (m - j - 1) * sizeof *vol);
				pms[j] = &src[i];
				vol[j] = v;
				break;
			}
		}
	}
	for(int i = 0; i < m; i++) if(pms[i]->left || pms[i]->loop){
		const Vec3d &pos = pms[i]->pos;
		double dist = (pms[i]->pos - view).len();

		/* get scalar product of the 'ear' vector and offset from the viewpoint */
		pan[i] = 128 * xh.sp(pos - view) / dist;
	}
	const volatile int interp = g_interpolate_sound_3d;
	for(int t = 0; t < maxt; t++){
		long tt0 = 0, tt1 = 0;
		for(int i = 0; i < m; i++){
			sounder3d &s = *pms[i];
			long lsrct = (t - s.delay) * s.ipitch;
#if INTERPOLATE_SOUND_3D
			long modt = lsrct % PITCHDIV;
#endif
			long srct = lsrct / PITCHDIV;
			if(s.loop && s.left < srct)
				srct -= s.size;
#if INTERPOLATE_SOUND_3D
			if(s.loop || 0 <= srct && srct + (interp && modt) < s.left)
#else
			if(s.loop || 0 <= srct && srct < s.left)
#endif
			{
				if(s.sampleBytes == 1){
					auto getf = [&](long srct, long pan){return ((long)s.src[srct] - 128) * vol[i] * (128 + pan) / voldiv;};
#if INTERPOLATE_SOUND_3D
					if(interp){
						tt0 += (getf(srct, -pan[i]) * (PITCHDIV - modt) + getf(srct+1, -pan[i]) * modt) / PITCHDIV;
						tt1 += (getf(srct, pan[i]) * (PITCHDIV - modt) + getf(srct+1, pan[i]) * modt) / PITCHDIV;
					}
					else
#endif
					{
						tt0 += getf(srct, -pan[i]);
						tt1 += getf(srct, pan[i]);
					}
				}
				else{
					auto getf = [&](long srct, long pan){return (long)pms[i]->src16[srct] * vol[i] / 256 * (128 + pan) / voldiv;};
#if INTERPOLATE_SOUND_3D
					if(interp){
						tt0 += (getf(srct, -pan[i]) * (PITCHDIV - modt) + getf(srct+1, -pan[i]) * modt) / PITCHDIV;
						tt1 += (getf(srct, pan[i]) * (PITCHDIV - modt) + getf(srct+1, pan[i]) * modt) / PITCHDIV;
					}
					else
#endif
					{
						tt0 += getf(srct, -pan[i]);
						tt1 += getf(srct, pan[i]);
					}
				}
			}
		}
#if SAMPLEBITS == 8
		tt0 = tt0 * wave_current_volume / 256 / 256;
		tt1 = tt1 * wave_current_volume / 256 / 256;
		tt0 += (dst[t] & 0xff) - 128;
		tt1 += ((dst[t] >> 8) & 0xff) - 128;
		dst[t] = (tt0 > 127 ? 255 : tt0 < -128 ? 0 : (BYTE)(tt0 + 128)) |
			((tt1 > 127 ? 255 : tt1 < -128 ? 0 : (BYTE)(tt1 + 128)) << 8);
#else
		tt0 = tt0 * wave_current_volume / 256;
		tt1 = tt1 * wave_current_volume / 256;
		tt0 += (short)dst[t * 2];
		tt1 += (short)dst[t * 2 + 1];
		dst[t * 2] = (tt0 > SHRT_MAX ? SHRT_MAX : tt0 < SHRT_MIN ? SHRT_MIN : (sbits)(tt0));
		dst[t * 2 + 1] = (tt1 > SHRT_MAX ? SHRT_MAX : tt1 < SHRT_MIN ? SHRT_MIN : (sbits)(tt1));
#endif
	}
}

static double loudness(const sounder3d *ms, const double listener[3]){
	return ms->left ? ms->vol / (1. + ms->attn * VECSDIST(ms->pos, listener)) : 0.;
}

static double g_listener[3];

static int louder(const sounder3d **a, const sounder3d **b){
	double la, lb;
	la = loudness(*a, g_listener);
	lb = loudness(*b, g_listener);
	return la < lb ? 1 : lb < la ? -1 : 0;
}

static void stopsound3d(sounder3d *src){
	// Differentiate serial number to prevent referrers from updating already stopped sound.
	// Note that addsound3d calls this prior to allocating new sound, so serial number 0
	// is never returned for a valid sound.
	src->serial++;
	src->left = 0;
	src->loop = false;
}

sounder3d::sounder3d(const SoundSource3D &s) : SoundSource3D(s), left(size), ipitch(calcPitch()){
	if(0 <= (long)s.delay)
		delay = s.delay;
	else if(left < -(long)s.delay){
		left += s.delay;
		delay = 0;
	}
	else
		left = 0;
	assert(0 <= (int)left);
}

int addsound3d(const SoundSource3D &s){
	sounder3d *added;
	int ret;
	assert(0 < s.sampleBytes);
	EnterCriticalSection(&gcs);

	// Find first msounder whose loop param is false
	do{
		added = &msounders[imsounder = (imsounder + 1) % numof(msounders)];
	}while(added->loop);

	stopsound3d(added);
	*added = s;
	ret = (added->serial << 16) | (added - msounders);
	LeaveCriticalSection(&gcs);
	return ret;
}

static unsigned sidDecode(int sid, short &serial){
	serial = sid >> 16;
	return sid & 0xffff;
}

template<typename T>
static int modifysound3d(int sid, T modifier){
	short serial;
	unsigned i = sidDecode(sid, serial);
	int ret = sid;
	if(numof(msounders) <= i)
		return -1;

	EnterCriticalSection(&gcs);
	assert(0 <= (int)msounders[i].left);
	if(msounders[i].serial != serial){
		ret = -1;
	}
	else{
		modifier(msounders[i]);
	}
	LeaveCriticalSection(&gcs);

	return ret;
}

// We use C++11's lambda expression extensively.
// This means the sound engine cannot be compiled with older compilers.
// We prefer productivity than backward-compatibility here since all production compilers
// would support C++11 at the time this software is released.
// What a big leap, considering this source was in C a few days ago.

int movesound3d(int sid, const Vec3d &pos){
	return modifysound3d(sid, [&pos](sounder3d &m){ m.pos = pos; });
}

int volumesound3d(int sid, double vol){
	return modifysound3d(sid, [&vol](sounder3d &m){ m.vol = vol; });
}

int pitchsound3d(int sid, double pitch){
	return modifysound3d(sid, [&pitch](sounder3d &m){ m.pitch = pitch; m.ipitch = m.calcPitch(); });
}

int stopsound3d(int sid){
	return modifysound3d(sid, [](sounder3d &m){ stopsound3d(&m); });
}

bool isEndSound3D(int sid){
	short serial;
	unsigned i = sidDecode(sid, serial);
	if(numof(msounders) <= i)
		return true;

	// Read-only access do not lock the critical section
	return msounders[i].serial != serial || 0 == msounders[i].left;
}

static Vec3d listener_pos(0,0,0);
static Vec3d listener_xhat(1,0,0);

void setListener(const Vec3d &pos, const Quatd &rot){
	EnterCriticalSection(&gcs);
	listener_pos = pos;
	listener_xhat = rot.trans(Vec3d(1,0,0));
	LeaveCriticalSection(&gcs);
}

double dwo;

void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT msg, DWORD ins, DWORD p1, DWORD p2){
	static int init = 0;
	static timemeas_t tm;
	static int k = 0;
	if(/*msg == WOM_OPEN ||*/ msg == WOM_DONE){
		WAVEHDR *pwho, *pwh;
		pwho = &whs[k];
		k = (k + 1) % 2;

/*		if(pwho->dwFlags & WHDR_DONE)
			OutputDebugString("WaveOut is done!\n");
		if(pwho->dwFlags & WHDR_PREPARED)
			OutputDebugString("WaveOut is prepared!\n");*/
		waveOutUnprepareHeader(hwo, pwho, sizeof *pwho);
//		OutputDebugString(msg == WOM_OPEN ? "WaveOut open!\n" : "WaveOut done!\n");

		dwo = TimeMeasLap(&tmwo);

//		EnterCriticalSection(&pc->cswave);

//		setmsounders(sounders, numof(sounders), msounders, numof(msounders));

		InterlockedExchange(&wave_current_volume, wave_volume);

		{
		int i, t, nes = 0; // number of effective sounders
		for(i = 0; i < numof(sounders); i++) if(sounders[i].left)
			nes++;
		if(!nes)
			initsound((BYTE*)pwho->lpData);
//			memset(pwho->lpData, 0x80, sizeof soundbuf[0]);
		else{
#if 1
			wavesum((sbits*)pwho->lpData, sounders, numof(soundbuf[0]), numof(sounders));
#else
			for(t = 0; t < numof(soundbuf[0]); t++){
				long tt = 0;
				for(i = 0; i < numof(sounders); i++){
					unsigned t2 = t * (sounders[i].pitch + 32) / 32;
					if(t2 < sounders[i].left)
						tt += ((long)sounders[i].src[t2] - 128) * sounders[i].vol / 256;
				}
				((BYTE*)pwho->lpData)[t] = tt > 127 ? 255 : tt < -128 ? 0 : (BYTE)(tt + 128);
			}
#endif

			// advance buffers
			for(i = 0; i < numof(sounders); i++){
				sounder &s = sounders[i];
				if(!s.left)
					continue;
				unsigned tt2 = numof(soundbuf[0]) * s.ipitch / PITCHDIV;
				if(s.delay < tt2){
					tt2 -= s.delay;
					s.delay = 0;
				}
				else{
					s.delay -= tt2;
					tt2 = 0;
				}
				if(s.left < tt2){
					if(!s.loops)
						s.left = 0;
					else{
						s.left = s.size;
						s.cur = s.src;
						s.loops--;
					}
				}
				else if(tt2){
					s.left -= tt2;
					s.cur += s.sampleBytes * tt2;
				}
			}
		}

		EnterCriticalSection(&gcs);
		{
			wavesumm8s((sbits*)pwho->lpData, msounders, numof(soundbuf[0]), numof(msounders), listener_pos, listener_xhat);

			// advance buffers
			for(i = 0; i < numof(msounders); i++){
				sounder3d &m = msounders[i];
				if(!m.left)
					continue;
				unsigned tt2 = numof(soundbuf[0]) * m.ipitch / PITCHDIV;
				assert(0 <= (int)m.left);
				if(m.delay < tt2){
					tt2 -= m.delay;
					m.delay = 0;
				}
				else{
					m.delay -= tt2;
					tt2 = 0;
				}
				if(m.left < tt2){
					if(m.loop){
						m.src -= m.sampleBytes * (m.size - tt2);
						m.left += m.size - tt2;
					}
					else
						m.left = 0;
				}
				else if(tt2){
					m.left -= tt2;
					m.src += m.sampleBytes * tt2;
				}
			}
		}
		if(listener_inwater){
			int i;
			for(i = 1; i < numof(soundbuf[0]); i++){
#if SAMPLEBITS == 16
				short (*pbuf)[2] = (short(*)[2])pwho->lpData;
				pbuf[i][0] = (pbuf[i][0] + pbuf[i-1][0]) / 2;
				pbuf[i][1] = (pbuf[i][1] + pbuf[i-1][1]) / 2;
#endif
			}
		}
		LeaveCriticalSection(&gcs);
		}
//		LeaveCriticalSection(&pc->cswave);

		pwho->lpData = (LPSTR)soundbuf[pwho - whs];
		pwho->dwBufferLength = sizeof soundbuf[0];
		pwho->dwFlags = 0;
		waveOutPrepareHeader(hwo, pwho, sizeof *pwh);
		pwho->dwFlags |= WHDR_BEGINLOOP | WHDR_ENDLOOP;
		pwho->dwLoops = 1;
		waveOutWrite(hwo, pwho, sizeof(*pwh));
		TimeMeasStart(&tm);
		if(!init){
			init = 1;
		}

	}
}

#if DEBUG_SOUND_WINDOW

static void DrawWave(HWAVEOUT hwo, HWND hWnd, HDC hdc, WPARAM wParam){
	SetBkColor(hdc, RGB(0, 0, 0));
	RECT rr;
	GetClientRect(hWnd, &rr);
	int wid = rr.right - rr.left;
	int hei = rr.bottom - rr.top;

	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rr, NULL, 0, NULL);

	HPEN hPen = CreatePen(PS_SOLID, 0, RGB(127,127,127));
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	MoveToEx(hdc, rr.left, (rr.top + rr.bottom) / 2, NULL);
	LineTo(hdc, rr.right, (rr.top + rr.bottom) / 2);

	// Print both channels
	for(int c = 0; c < 2; c++){
		HPEN hGraphPen = CreatePen(PS_SOLID, 0, c == 0 ? RGB(255,0,0) : RGB(0,255,0));
		SelectObject(hdc, hGraphPen);
		MoveToEx(hdc, 0, hei / 2, NULL);
		int count = numof(soundbuf[0]);
		for(int i = 0; i < count; i++){
			int s = i * numof(soundbuf[0]) / count / 2 * 2 + c;
			int x = wid * i / count;
			LineTo(hdc, x, hei / 2 + ((sbits*)((WAVEHDR*)wParam)->lpData)[s]
				* hei / 2 / (SAMPLEBITS == 8 ? CHAR_MAX : INT16_MAX));
		}
		SelectObject(hdc, hOldPen);
		DeleteObject(hGraphPen);
	}
	DeleteObject(hPen);
}

#endif

int g_debug_sound = 0;

static DWORD WINAPI WaveOutThread(HWAVEOUT *phwo){
	MSG msg;

	// make sure to get the message queue active
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	while(GetMessage(&msg, NULL, 0, 0)){
#if DEBUG_SOUND_WINDOW
		if(g_debug_sound && msg.message == WOM_DONE){
			HDC hdc = GetDC(hWndWave);
			DrawWave(*phwo, hWndWave, hdc, msg.lParam);
			ReleaseDC(hWndWave, hdc);
		}
#endif
		WaveOutProc(*phwo, msg.message, (DWORD)0, msg.lParam, msg.wParam);
	}

	DestroyWindow(hWndWave);

	return 0;
}

}
