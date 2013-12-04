#include "audio/wavemixer.h"
#include "wavesum.h"
extern "C"{
#include "clib/timemeas.h"
#include "clib/rseq.h"
#include "clib/avec3.h"
}
#include <mmsystem.h>
#include <assert.h>

#define numof(a) (sizeof(a)/sizeof*(a))

#define SAMPLERATE 22050
#define SAMPLEBITS 16
#define MAXMIX 32
#define EPSILON 0.001

namespace audio{

static HWAVEOUT hwo;

#if SAMPLEBITS == 8
typedef BYTE sbits;
#elif SAMPLEBITS == 16
typedef WORD sbits;
#else
#error SAMPLEBITS neither 8 nor 16!
#endif

long wave_volume = 255;
static volatile long wave_current_volume = 255;

static sbits soundbuf[2][2048][2];
static WAVEHDR whs[2];
timemeas_t tmwo;
LONG listener_inwater = 0;
static CRITICAL_SECTION gcs;

HANDLE CreateWaveOutThread(void){
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

typedef struct sounder8 sounder;
typedef struct movablesounder8 msounder;

static sounder sounders[32];
static int isounder = 0;

static msounder msounders[128];
static short imsounder = 0;

int addsound(const unsigned char *src, size_t size, size_t delay, short vol, short pitch, char pan, unsigned short loops, char priority){
	int i;
	sounder *ps;
	if(vol < 16 || !size)
		return -1;
	assert(-32 < pitch);
	for(i = 0; i < numof(sounders); i++) if(sounders[i].left == 0 || sounders[i].priority < priority
		|| sounders[i].priority == priority && (long)delay - SAMPLERATE / vol < (long)sounders[i].delay - SAMPLERATE / sounders[i].vol){
		isounder = i;
		break;
	}
	if(i == numof(sounders))
		return -1;
	ps = &sounders[isounder];
	ps->src =
	ps->cur = src;
	ps->size = 
	ps->left = size;
	ps->delay = delay;
	ps->vol = vol;
	ps->pitch = pitch;
	ps->loops = loops;
	ps->pan = pan;
	ps->priority = priority;
	isounder = (isounder + 1) % numof(sounders);
	return isounder;
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

int maddsound(const unsigned char *src, size_t size, size_t delay, const double pos[3], double vol, double attn){
	msounder *s;
	s = &msounders[imsounder];
	s->src = src;
	s->left = size;
	s->delay = delay;
	s->serial++;
	VECCPY(s->pos, pos);
	s->vol = vol;
	s->attn = attn;
	imsounder = (imsounder + 1) % numof(msounders);
	return imsounder & (s->serial << 16); /* encode the pointer that it includes the serial number. */
}

void initsound(void *pv){
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

static void wavesum8s(unsigned short *dst, struct sounder8 src[], unsigned long maxt, int nsrc){
	int t;
	for(t = 0; t < maxt; t++){
		long tt0 = 0, tt1 = 0;
		int i;
		for(i = 0; i < nsrc; i++){
			size_t left = src[i].pitch == 0 ? src[i].left : src[i].left * 32 / (src[i].pitch + 32);
			if(src[i].delay < t && t < left + src[i].delay){
				long value = src[i].vol * (src[i].pitch == 0 ? ((long)src[i].cur[t - src[i].delay] - 128) : src[i].cur[t * (src[i].pitch + 32) / 32 - src[i].delay] - 128);
				tt0 += value * (128 + src[i].pan);
				tt1 += value * (127 - src[i].pan);
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

static void wavesumm8s(unsigned short *dst, struct movablesounder8 src[], unsigned long maxt, int nsrc, const Vec3d &view, const Vec3d &xh){
	msounder *pms[MAXMIX]; // Temprary msounder array for sorting by calculated volume
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
	for(int t = 0; t < maxt; t++){
		long tt0 = 0, tt1 = 0;
		for(int i = 0; i < m; i++){
			long srct = (t - pms[i]->delay) * pms[i]->pitch / 256;
			if(pms[i]->loop && pms[i]->left < srct)
				srct -= pms[i]->size;
			if(pms[i]->loop || 0 <= srct && srct < pms[i]->left){
				tt0 += ((long)pms[i]->src[srct] - 128) * vol[i] * (128 - pan[i]) / voldiv;
				tt1 += ((long)pms[i]->src[srct] - 128) * vol[i] * (pan[i] + 128) / voldiv;
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

static double loudness(const msounder *ms, const double listener[3]){
	return ms->left ? ms->vol / (1. + ms->attn * VECSDIST(ms->pos, listener)) : 0.;
}

static double g_listener[3];

static int louder(const msounder **a, const msounder **b){
	double la, lb;
	la = loudness(*a, g_listener);
	lb = loudness(*b, g_listener);
	return la < lb ? 1 : lb < la ? -1 : 0;
}

static int stopsound3d(msounder *src){
	if(src->serial < numof(sounders)){
		sounders[src->serial].left = 0;
		sounders[src->serial].loops = 0;
		src->serial = numof(sounders);
		return 1;
	}
	else
		return 0;
}

static void setmsounders(sounder *dst, int ndst, msounder *src, int nsrc){
	int i;
	msounder *list[numof(msounders)];
	assert(nsrc <= numof(list));
	for(i = 0; i < nsrc; i++)
		list[i] = &src[i];
	qsort(list, numof(list), sizeof(*list), (int(*)(const void*, const void*))louder);
	for(i = 0; i < nsrc; i++){
		unsigned short ld;
		msounder *ms = list[i];
		ld = 256 * loudness(ms, g_listener);
		if(numof(sounders) <= list[i]->serial){
			int j;
			for(j = 0; j < ndst; j++) if(!dst[j].priority && dst[j].vol < ld){
				list[i]->serial = j;
				break;
			}
		}
		else if(ld <= 0)
			list[i]->serial = numof(sounders);
		if(list[i]->serial < numof(sounders)){
			sounder *s = &dst[list[i]->serial];
			s->src = list[i]->src;
		}
		/* set contents of sounder by msounder */
		/*ms->src += */
	}
}

int addsound3d(const SoundSource &s){
	msounder *added;
	int ret;
	EnterCriticalSection(&gcs);

	// Find first msounder whose loop param is false
	do{
		added = &msounders[imsounder = (imsounder + 1) % numof(msounders)];
	}while(added->loop);

	stopsound3d(added);
	*static_cast<SoundSource*>(added) = s;
	added->left = s.size;
	if(0 <= (long)s.delay)
		added->delay = s.delay;
	else if(added->left < -(long)s.delay){
		added->left += s.delay;
		added->delay = 0;
	}
	else
		added->left = 0;
	assert(0 <= (int)added->left);
	added->serial++;
	added->pitch = (unsigned short)(s.pitch * s.srcpitch / SAMPLERATE);
	ret = (added->serial << 16) | (added - msounders);
	LeaveCriticalSection(&gcs);
	return ret;
}

template<typename T>
static int modifysound3d(int sid, T modifier){
	unsigned i = sid & 0xffff;
	short serial = sid >> 16;
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

	return sid;
}

// We use C++11's lambda expression extensively.
// This means the sound engine cannot be compiled with older compilers.
// We prefer productivity than backward-compatibility here since all production compilers
// would support C++11 at the time this software is released.
// What a big leap, considering this source was in C a few days ago.

int movesound3d(int sid, const Vec3d &pos){
	return modifysound3d(sid, [&pos](msounder &m){ m.pos = pos; });
}

int volumesound3d(int sid, double vol){
	return modifysound3d(sid, [&vol](msounder &m){ m.vol = vol; });
}

int pitchsound3d(int sid, double pitch){
	return modifysound3d(sid, [&pitch](msounder &m){ m.pitch = max(1, long(256 * pitch)); });
}

int stopsound3d(int sid){
	return modifysound3d(sid, [](msounder &m){ stopsound3d(&m); });
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
			wavesum8s((sbits*)pwho->lpData, sounders, numof(soundbuf[0]), numof(sounders));
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
			for(i = 0; i < numof(sounders); i++) if(sounders[i].left){
				unsigned tt2 = numof(soundbuf[0]) * (sounders[i].pitch + 32) / 32;
				if(sounders[i].delay < tt2){
					tt2 -= sounders[i].delay;
					sounders[i].delay = 0;
				}
				else{
					sounders[i].delay -= tt2;
					tt2 = 0;
				}
				if(sounders[i].left < tt2){
					if(!sounders[i].loops)
						sounders[i].left = 0;
					else{
						sounders[i].left = sounders[i].size;
						sounders[i].cur = sounders[i].src;
						sounders[i].loops--;
					}
				}
				else if(tt2){
					sounders[i].left -= tt2;
					sounders[i].cur += tt2;
				}
			}
		}

		EnterCriticalSection(&gcs);
		{
			wavesumm8s((sbits*)pwho->lpData, msounders, numof(soundbuf[0]), numof(msounders), listener_pos, listener_xhat);

			// advance buffers
			for(i = 0; i < numof(msounders); i++){
				msounder &m = msounders[i];
				if(!m.left)
					continue;
				unsigned tt2 = numof(soundbuf[0]) * m.pitch / 256;
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
						m.src -= m.size - tt2;
						m.left += m.size - tt2;
					}
					else
						m.left = 0;
				}
				else if(tt2){
					m.left -= tt2;
					m.src += tt2;
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
#if 0
static void CALLBACK WaveOutPoster(HWAVEOUT hwo, UINT msg, DWORD ins, DWORD p1, DWORD p2){
	PostMessage(((struct client*)ins)->w, MM_WOM_DONE, p1, p2);
}

static void DrawWave(struct client *pc, HDC hdc, WPARAM wParam){
#if 0
	int k;
	MMTIME mmtime;
	RECT rr = {320, 320, 320 + 32, 320 + 32};
	mmtime.wType = TIME_SAMPLES;
	waveOutGetPosition(pc->hwo, &mmtime, sizeof mmtime);
	SetBkColor(hdc, !mmtime.u.sample ? RGB(255,0,0) : RGB(0,255,0));
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rr, NULL, 0, NULL);
	rr.left += 32;
	rr.right += 32;
	for(k = 0; k < 2; k++){
		SetBkColor(hdc, (~(whs[k].dwFlags | ~(/*WHDR_PREPARED |*/ WHDR_DONE))) | (whs[k].dwFlags & WHDR_INQUEUE) ? RGB(255,0,0) : RGB(0,255,0));
		rr.left += k * 32;
		rr.right += k * 32;
		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rr, NULL, 0, NULL);
	}
#endif
#if 1
	int i, t;
	{
		MMTIME mmt;
		mmt.wType = TIME_SAMPLES;
		waveOutGetPosition(pc->hwo, &mmt, sizeof mmt);
		t = mmt.u.sample % numof(soundbuf[0]);
	}
	SetBkColor(hdc, RGB(127, 127, 127));
	RECT rr = {320, 320, 320 + 256, 320 + 256};
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rr, NULL, 0, NULL);
	for(i = 0; i < 256; i++){
		if(i == t * numof(soundbuf[0]) / 256)
			SelectObject(hdc, GetStockObject(WHITE_PEN));
		MoveToEx(hdc, 320 + i, 320 + 128, NULL);
		LineTo(hdc, 320 + i, 320 + ((BYTE*)((WAVEHDR*)wParam)->lpData)[i * numof(soundbuf[0]) / 256]);
	}
#endif
}
#endif

DWORD WINAPI WaveOutThread(HWAVEOUT *phwo){
	MSG msg;

	// make sure to get the message queue active
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	while(GetMessage(&msg, NULL, 0, 0)){
/*		if(msg.message == WOM_DONE){
			HDC hdc = GetDC(pc->w);
			DrawWave(pc, hdc, msg.lParam);
			ReleaseDC(pc->w, hdc);
		}*/
		WaveOutProc(*phwo, msg.message, (DWORD)0, msg.lParam, msg.wParam);
	}
	return 0;
}

}
