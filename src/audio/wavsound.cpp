#include "audio/wavsound.h"
#include "audio/wavemixer.h"
extern "C"{
#include "clib/c.h"
#include "clib/zip/UnZip.h"
}
#if defined _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
//#include <dsound.h>
#include "clib/avec3.h"

#define SAMPLERATE 11025

namespace audio{

double wave_sonic_speed = 340.;

#if 1
/*
static void CALLBACK waveSoundStopper(
	HWAVEOUT hwo,
	UINT msg,         
	DWORD dwInstance,  
	DWORD dwParam1,    
	DWORD dwParam2)
{
	if(msg == WOM_DONE){
 		WAVEHDR *pwh;
		HGLOBAL hg;
		pwh = (WAVEHDR*)dwParam1;
		waveOutClose(hwo);
		hg = pwh->dwUser;
		GlobalUnlock(hg);
		GlobalFree(hg);
	}
}
*/
static LRESULT CALLBACK SoundStopper(HWND hw, UINT msg, WPARAM wp, LPARAM lp){
	if(msg == WOM_DONE){
 		WAVEHDR *pwh;
		HGLOBAL hg;
		pwh = (WAVEHDR*)lp;
        waveOutClose((HWAVEOUT)wp);
		hg = (HGLOBAL)pwh->dwUser;
		GlobalUnlock(hg);
		GlobalFree(hg);
	}
	return TRUE;
}

struct RIFFHEADER {
	char ckid[4];
	DWORD cksize;
	char fccType[4];
	char fmtType[4];
	DWORD chunkLength;
	WORD fmt;
	WORD channels;
	DWORD samplesPerSec;
	DWORD bytesPerSec;
	WORD bytesPerSample;
	WORD bitsPerSample;
};

typedef struct wavecache{
	char fname[MAX_PATH];
	WAVEFORMATEX format;
	WAVEHDR head;
	struct wavecache *hi, *lo;
} wavc_t;


static void playWave(wavc_t *wc, HWND hwndApp){
    HWAVEOUT    hWaveOut; 
    HGLOBAL     hWaveHdr; 
	LPWAVEHDR	lpWaveHdr;
    UINT        wResult; 

    // Open a waveform device for output using window callback. 

	if (waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
					&wc->format, 
                    (LONG)hwndApp, 0L, CALLBACK_WINDOW))
/*    if (waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
                    pFormat, 
                    (LONG)waveSoundStopper, 0L, CALLBACK_FUNCTION)) */
    { 
        MessageBox(NULL, 
                   "Failed to open waveform output device.", 
                   NULL, MB_OK | MB_ICONEXCLAMATION); 
/*        LocalUnlock(hFormat); 
        LocalFree(hFormat); 
        mmioClose(hmmio, 0); */
        return; 
    }
 
    hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
        (DWORD) sizeof(WAVEHDR)); 
    if (hWaveHdr == NULL) 
    { 
        MessageBox(NULL, "Not enough memory for header.", 
            NULL, MB_OK | MB_ICONEXCLAMATION); 
        return; 
    } 
 
    lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 
    if (lpWaveHdr == NULL) 
    { 
        MessageBox(NULL, 
            "Failed to lock memory for header.", 
            NULL, MB_OK | MB_ICONEXCLAMATION); 
        return; 
    }

	memcpy(lpWaveHdr, &wc->head, sizeof(WAVEHDR));
	lpWaveHdr->dwUser = (DWORD)hWaveHdr;

	waveOutPrepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR)); 
 
    // Now the data block can be sent to the output device. The 
    // waveOutWrite function returns immediately and waveform 
    // data is sent to the output device in the background. 

/*	waveOutSetVolume(hWaveOut, UINT_MAX);*/
	wResult = waveOutWrite(hWaveOut, lpWaveHdr, sizeof(WAVEHDR)); 
    if (wResult != 0) 
    { 
        waveOutUnprepareHeader(hWaveOut, lpWaveHdr, 
                               sizeof(WAVEHDR)); 
		GlobalUnlock(hWaveHdr);
		GlobalFree(hWaveHdr);
/*        GlobalUnlock( hData); 
        GlobalFree(hData); */
        MessageBox(NULL, "Failed to write block to device", 
                   NULL, MB_OK | MB_ICONEXCLAMATION); 
        return; 
    } 
}


typedef struct sMPOS{
	unsigned char *org;
	unsigned char *cur;
	unsigned char *end;
} MPOS;

static size_t mread(void *buf, size_t s, size_t c, MPOS *fp){
	size_t ret = MIN((fp->end - fp->cur), s * c) / s;
	memcpy(buf, fp->cur, s * ret);
	fp->cur += s * ret;
	return ret;
}

static int mseek(MPOS *fp, long v, int whence){
	switch(whence){
		case SEEK_SET: fp->cur = &fp->org[v]; return 0;
		case SEEK_CUR: fp->cur += v; return 0;
		case SEEK_END: fp->cur = &fp->end[v]; return 0;
	}
	return 1;
}
 
wavc_t *cacheWaveDataV(int fromMemory, FILE *hmmio)
{
	MPOS mp;
	wavc_t *ret;
	struct RIFFHEADER head;
    LPWAVEHDR   lpWaveHdr; 
  /*  HMMIO       hmmio; */
    HANDLE      hFormat; 
    DWORD data, dwDataSize; 
	HANDLE hData  = NULL;  // handle of waveform data memory 
	HPSTR  lpData = NULL;  // pointer to waveform data memory 
	size_t (*pread)(void *, size_t, size_t, FILE*) = fromMemory ? (size_t (*)(void *, size_t, size_t, FILE*))mread : fread;
	int (*pseek)(FILE *, long, int) = fromMemory ? (int (*)(FILE *, long, int))mseek : fseek;

	if(!pread(&head, sizeof head, 1, hmmio)
		|| memcmp(head.ckid, "RIFF", sizeof head.ckid)
		|| memcmp(head.fccType, "WAVE", sizeof head.fccType)
		|| memcmp(head.fmtType, "fmt ", sizeof head.fmtType)
		|| head.channels != 1
		|| head.bitsPerSample != 8){
		return NULL;
	}
	pseek(hmmio, 20 + head.chunkLength, SEEK_SET);
	do{
		pread(&data, sizeof data, 1, hmmio);
		if(!memcmp(&data, "data", sizeof data))
			break;
		pread(&dwDataSize, sizeof dwDataSize, 1, hmmio);
		pseek(hmmio, dwDataSize, SEEK_CUR);
	} while(1);

	// Allocate and lock memory for the waveform data. 
 
	if(pread(&dwDataSize, sizeof dwDataSize, 1, hmmio) <= 0){
		return NULL;
	}

	ret = (wavc_t*)malloc(sizeof(wavc_t) + dwDataSize);

	ret->format.wFormatTag = WAVE_FORMAT_PCM;
	ret->format.nChannels = head.channels;
	ret->format.nSamplesPerSec = head.samplesPerSec;
	ret->format.nAvgBytesPerSec = head.bytesPerSec;
	ret->format.nBlockAlign = head.channels * head.bitsPerSample / 8;
	ret->format.wBitsPerSample = head.bitsPerSample;
	ret->format.cbSize = 0;

#if 1
	lpData = (HPSTR)&ret[1];
#else
    hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, dwDataSize ); 
    if (!hData)
    { 
        MessageBox(NULL, "Out of memory.", 
                   NULL, MB_OK | MB_ICONEXCLAMATION); 
     /*   mmioClose(hmmio, 0); */
		fclose(hmmio);
        return; 
    } 
    if ((lpData = GlobalLock(hData)) == NULL) 
    { 
        MessageBox(NULL, "Failed to lock memory for data chunk.", 
                   NULL, MB_OK | MB_ICONEXCLAMATION); 
        GlobalFree(hData); 
/*        mmioClose(hmmio, 0); */
		fclose(hmmio);
        return; 
    } 
#endif
 
    // Read the waveform data subchunk. 
 
    if(pread(lpData, 1, dwDataSize, hmmio) != dwDataSize) 
    { 
		MessageBox(NULL, ferror(hmmio) ? "Failed to read data chunk. Error" :feof(hmmio) ? "Failed to read data chunk. EOF" : "Failed to read data chunk.", 
                   NULL, MB_OK | MB_ICONEXCLAMATION); 
/*        GlobalUnlock(hData); 
        GlobalFree(hData); */
/*        mmioClose(hmmio, 0); */
		free(ret);
        return NULL; 
    } 
 
    // Allocate and lock memory for the header. 

#if 1
	lpWaveHdr = &ret->head;
#else
    hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
        (DWORD) sizeof(WAVEHDR)); 
    if (hWaveHdr == NULL) 
    { 
        GlobalUnlock(hData); 
        GlobalFree(hData); 
        MessageBox(NULL, "Not enough memory for header.", 
            NULL, MB_OK | MB_ICONEXCLAMATION); 
		fclose(hmmio);
        return; 
    } 
 
    lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 
    if (lpWaveHdr == NULL) 
    { 
        GlobalUnlock(hData); 
        GlobalFree(hData); 
        MessageBox(NULL, 
            "Failed to lock memory for header.", 
            NULL, MB_OK | MB_ICONEXCLAMATION); 
		fclose(hmmio);
       return; 
    } 
#endif

    // After allocation, set up and prepare header. 
 
    lpWaveHdr->lpData = lpData; 
    lpWaveHdr->dwBufferLength = dwDataSize; 
    lpWaveHdr->dwFlags = 0L; 
    lpWaveHdr->dwLoops = 0L; 
 
	return ret;
}

wavc_t *cacheWaveData(const char *fname){
	wavc_t *ret;
	FILE *hmmio;
	hmmio = fopen(fname, "rb");
	if(!hmmio){
		const char *p, *sup = fname;
		do{
			char zipname[256];
			p = strchr(sup, '/');
			if(!p && !(p = strchr(sup, '\\')))
				break;
			strncpy(zipname, fname, p - fname);
			zipname[p - fname] = '\0';
			if(INVALID_FILE_ATTRIBUTES != GetFileAttributes(zipname)){
				void *unzipped;
				unsigned long size;
				MPOS mp;
				unzipped = ZipUnZip(zipname, p+1, &size);
				if(unzipped){
					mp.cur = mp.org = (unsigned char*)unzipped;
					mp.end = &mp.cur[size];
					ret = cacheWaveDataV(1, (FILE*)&mp);
					ZipFree(unzipped);
					return ret;
				}
			}
			sup = p + 1;
		}while(1);
		return NULL;
	}
	ret = cacheWaveDataV(0, hmmio);
	fclose(hmmio);
	return ret;
}

wavc_t *cacheWaveDataZ(const char *zipname, const char *fname){
	wavc_t *ret;
	unsigned long size;
	MPOS fp;
	fp.org = fp.cur = (unsigned char*)ZipUnZip(zipname, fname, &size);
	if(!fp.org)
		return NULL;
	fp.end = &fp.cur[size];
	ret = cacheWaveDataV(1, (FILE*)&fp);
	ZipFree(fp.org);
	return ret;
}

int playWaveCustom(const char *lpszWAVEFileName, size_t delay, unsigned short vol, unsigned short pitch, signed char pan, unsigned short loops, char priority){
	static wavc_t *root = NULL;
	wavc_t **node;
	if(vol < 16)
		return 0;
	for(node = &root; *node;){
		int res;
		res = strncmp((*node)->fname, lpszWAVEFileName, sizeof (*node)->fname);
		if(res < 0) node = &(*node)->hi;
		else if(0 < res) node = &(*node)->lo;
		else break;
	}
	if(!*node){
		DWORD data;
		FILE *fp;

		if(!(*node = cacheWaveData(lpszWAVEFileName))){
			return 0;
		}
		strncpy((*node)->fname, lpszWAVEFileName, sizeof(*node)->fname);
		(*node)->hi = (*node)->lo = NULL;
	}

	addsound((unsigned char*)(*node)->head.lpData, (*node)->head.dwBufferLength, delay, vol, pitch, pan, loops, priority);

	return 1;
}

int playWave3DCustom(const char *lpszWAVEFileName, const Vec3d &pos, size_t delay, double vol, double attn, unsigned short pitch, bool loop){
	static wavc_t *root = NULL;
	wavc_t **node;
	if(!lpszWAVEFileName) /* allow NULL pointer as file name */
		return 0;
	for(node = &root; *node;){
		int res;
		res = strncmp((*node)->fname, lpszWAVEFileName, sizeof (*node)->fname);
		if(res < 0) node = &(*node)->hi;
		else if(0 < res) node = &(*node)->lo;
		else break;
	}
	if(!*node){
		DWORD data;
		FILE *fp;

		if(!(*node = cacheWaveData(lpszWAVEFileName))){
			return 0;
		}
		strncpy((*node)->fname, lpszWAVEFileName, sizeof(*node)->fname);
		(*node)->hi = (*node)->lo = NULL;
	}

	SoundSource ss;
	ss.src = (unsigned char*)(*node)->head.lpData;
	ss.size = (*node)->head.dwBufferLength;
	ss.delay = delay;
	ss.vol = vol;
	ss.attn = attn;
	ss.pos = pos;
	ss.pitch = pitch;
	ss.srcpitch = (*node)->format.nSamplesPerSec;
	ss.loop = loop;
	return addsound3d(ss);
}

int playWave3D(const char *fname, const Vec3d &src, double vol, double attn, double delay, bool loop, unsigned short pitch){
	extern double dwo;
	return playWave3DCustom(fname, src, delay < dwo ? 0 : (size_t)(SAMPLERATE * (delay - dwo)), vol, attn, pitch, loop);
}

int playMemoryWave3D(const unsigned char *wav, size_t size, short pitch, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay){
	extern double dwo;
	avec3_t delta, xhat;
	double sdist, dist;
	VECSUB(delta, src, org);
	sdist = VECSLEN(delta);
	dist = sqrt(sdist);
/*	xhat[0] = -cos(pyr[1]);
	xhat[1] = 0;
	xhat[2] =  sin(pyr[1]);*/
/*	return addsound(wav, size, (size_t)(SAMPLERATE * (dist / wave_sonic_speed)), (unsigned char)(256 * vol / (1. + sdist / attn)), pitch, -128 * VECSP(delta, xhat) / dist, 0, 0);*/
	SoundSource ss;
	ss.src = wav;
	ss.size = size;
	ss.delay = delay < dwo ? 0 : (size_t)(SAMPLERATE * (delay - dwo/* + dist / wave_sonic_speed*/));
	ss.vol = vol;
	ss.attn = attn;
	ss.pos = src;
	ss.pitch = pitch + 256;
	ss.srcpitch = 8000;
	ss.loop = false;
	return addsound3d(ss);
}

int playMovableWave3D(const char *fname, const double src[3], double vol, double attn){
	return 0;
}


int playWAVEFile(const char *lpszWAVEFileName)
{
#if 1
	return 0;
#else
	static init = 0;
	static HWND hwStopper;
	static wavc_t *root = NULL;
	wavc_t **node;
	HANDLE hfile;
	HMMIO hm;
	MMIOINFO mi;
	DWORD read;

	if(!init){
		WNDCLASS wc = {
			0, /* UINT       style; */
			SoundStopper, /*WNDPROC    lpfnWndProc; */
			0, /*int        cbClsExtra; */
			0, /*int        cbWndExtra; */
			NULL, /*HINSTANCE  hInstance; */
			NULL, /*HICON      hIcon; */
			NULL, /*HCURSOR    hCursor; */
			NULL, /*HBRUSH     hbrBackground; */
			NULL, /*LPCTSTR    lpszMenuName; */
			"SoundStopperClass", /*LPCTSTR    lpszClassName; */
		}; 
		ATOM atom;
#if 0
		IDirectSound *ds;
		DirectSoundCreate(NULL, &ds, NULL);
#endif
/*		ds->CreateSoundBuffer(*/
		wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);

		if(!(atom = RegisterClass(&wc)) ||
			!(hwStopper = CreateWindow((LPCTSTR)atom, "SoundStopper", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL))){
			return 1;
		}
		init = 1;
	}

	for(node = &root; *node;){
		int res;
		res = strncmp((*node)->fname, lpszWAVEFileName, sizeof (*node)->fname);
		if(res < 0) node = &(*node)->hi;
		else if(0 < res) node = &(*node)->lo;
		else break;
	}
	if(!*node){
		DWORD data;
		FILE *fp;

	/*	hfile = CreateFile(lpszWAVEFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		ReadFile(hfile, &head, sizeof head, &read, NULL);
		if(!memcmp(head.ckid, "RIFF", sizeof head.ckid) && !memcmp(head.fccType, "WAVE", sizeof head.fccType) && !memcmp(head.fmtType, "fmt ", sizeof head.fmtType)){
			DWORD data;
			ReadFile(hfile, &data, sizeof data, &read, NULL);
			if(!memcmp(&data, "data", sizeof data))
				WriteWaveData(&head, hfile, hwStopper);
		}
		CloseHandle(hfile);*/
		if(!(*node = cacheWaveData(lpszWAVEFileName))){
			return 0;
		}
		strncpy((*node)->fname, lpszWAVEFileName, sizeof(*node)->fname);
		(*node)->hi = (*node)->lo = NULL;
	}

	playWave(*node, hwStopper);
#endif
}

#else

static void MsgBoxErr(void){
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
	// Free the buffer.
	LocalFree( lpMsgBuf );
}

    static UINT wDeviceID;

static LRESULT CALLBACK SoundStopper(HWND hw, UINT msg, WPARAM wp, LPARAM lp){
	if(msg == MM_MCINOTIFY || msg == WM_DESTROY)
        mciSendCommand(lp, MCI_CLOSE, 0, NULL);
}

// Plays a specified waveform-audio file using MCI_OPEN and MCI_PLAY. 
// Returns when playback begins. Returns 0L on success, otherwise 
// returns an MCI error code.

int playWAVEFile(const char *lpszWAVEFileName)
{
	static int init = 0;
	static HWND hwStopper;
	static int counter = 0;
    DWORD dwReturn;
    MCI_OPEN_PARMS mciOpenParms;
    MCI_PLAY_PARMS mciPlayParms;

    // Open the device by specifying the device and filename.
    // MCI will choose a device capable of playing the specified file.

	if(!init){
		WNDCLASS wc = {
			0, /* UINT       style; */
			SoundStopper, /*WNDPROC    lpfnWndProc; */
			0, /*int        cbClsExtra; */
			0, /*int        cbWndExtra; */
			NULL, /*HINSTANCE  hInstance; */
			NULL, /*HICON      hIcon; */
			NULL, /*HCURSOR    hCursor; */
			NULL, /*HBRUSH     hbrBackground; */
			NULL, /*LPCTSTR    lpszMenuName; */
			"SoundStopperClass", /*LPCTSTR    lpszClassName; */
		}; 
		ATOM atom;
		wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);;

		if(!(atom = RegisterClass(&wc)) ||
			!(hwStopper = CreateWindow((LPCTSTR)atom, "SoundStopper", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL))){
			MsgBoxErr();
			return 1;
		}
		init = 1;
	}

	{
		WCHAR alias[5];
		alias[0] = '0' + counter % 10;
		alias[1] = '0' + counter / 10 % 10;
		alias[2] = '0' + counter / 10 / 10 % 10;
		alias[3] = '\0';
		counter++;
		mciOpenParms.lpstrDeviceType = "waveaudio";
		mciOpenParms.lpstrElementName = lpszWAVEFileName;
		mciOpenParms.lpstrAlias = alias;
		if (dwReturn = mciSendCommand(0, MCI_OPEN,
		MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS, 
		(DWORD)(LPVOID) &mciOpenParms))
		{
			// Failed to open device. Don't close it; just return error.
			return (dwReturn);
		}
		// The device opened successfully; get the device ID.
		wDeviceID = mciOpenParms.wDeviceID;
	}
	{
		UINT num;
		DWORD vol;
		num = waveOutGetNumDevs();
		if(num)
			auxGetVolume(0, &vol);
		num;
	}

    // Begin playback. The window procedure function for the parent 
    // window will be notified with an MM_MCINOTIFY message when 
    // playback is complete. At this time, the window procedure closes 
    // the device.

    mciPlayParms.dwCallback = (DWORD) hwStopper;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY, 
        (DWORD)(LPVOID) &mciPlayParms))
    {
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
        return (dwReturn);
    }

    return (0L);
}

#endif

#else
/* I'm sorry I don't know how to play it */
int playWAVEFile(const char *fname){fname; return 0;}
int playWaveCustom(const char *fname, unsigned short, unsigned short, char pan){fname, pan; return 0;}
#endif

}
