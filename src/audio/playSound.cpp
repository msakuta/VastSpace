#define USE_OGGVORBIS 1

#include "audio/playSound.h"
#include "audio/wavemixer.h"
#include "cmd.h"
extern "C"{
#include "clib/c.h"
#include "clib/zip/UnZip.h"
}
#if defined _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <vector>
//#include <dsound.h>
extern "C"{
#include "clib/avec3.h"
}

#if USE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

#define SAMPLERATE 11025

namespace audio{

double wave_sonic_speed = 340.;

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
	const char *p = strrchr(fname, '.');

#if USE_OGGVORBIS
	// If the file ends with .ogg extension, try interpreting as Ogg Vorbis.
	if(p && !strcmp(p, ".ogg")){
		static const int bytesPerSample = 2; // Preferred sample byte size
		OggVorbis_File vf;
		int error = ov_fopen(fname, &vf);

		if(error != 0){
			switch(error){ // Report file errors
			case OV_EREAD:       CmdPrintf("Ogg Vorbis OV_EREAD: %s", fname); break;
			case OV_ENOTVORBIS:  CmdPrintf("Ogg Vorbis OV_ENOTVORBIS: %s", fname); break;
			case OV_EVERSION:    CmdPrintf("Ogg Vorbis OV_EVERSION: %s", fname); break;
			case OV_EBADHEADER:  CmdPrintf("Ogg Vorbis OV_EBADHEADER: %s", fname); break;
			case OV_EFAULT:      CmdPrintf("Ogg Vorbis OV_EFAULT: %s", fname); break;
			default:             CmdPrintf("Ogg Vorbis Unknown Error: %s", fname); break;
			}
			return nullptr;
		}

		vorbis_info *vi = ov_info( &vf, -1 );
		if(!vi)
			return nullptr;

		// Only monaural or stereo sound sources are allowed as sound effects.
		// Sounds having more than 2 channels (surround encoding) are not supported (who would want it?).
		if(2 < vi->channels){
			ov_clear( &vf );
			return nullptr;
		}

		// Read into the buffer
		std::vector<char> buffer;
		long comSize = 0;
		while(true){
			static const long fetch_size = 4096;
			buffer.resize(comSize + fetch_size);
			int current_section;
			long bytes_read = ov_read(&vf, &buffer[comSize], fetch_size, 0, bytesPerSample, bytesPerSample == 1 ? 0 : 1, &current_section);
			if(bytes_read == 0)
				break;
//			buffer.resize(comSize + bytes_read);
//			memcpy(&buffer[comSize], tmpBuffer, bytes_read);
			comSize += bytes_read;
		}

		if(!comSize){
			ov_clear( &vf );
			return nullptr;
		}

		// Allocate wave cache entry for returning.
		wavc_t *ret = (wavc_t*)malloc(sizeof(wavc_t) + comSize);
		ret->format.wFormatTag = WAVE_FORMAT_PCM;
		ret->format.nChannels = vi->channels;
		ret->format.nSamplesPerSec = vi->rate;
		ret->format.nAvgBytesPerSec = vi->bitrate_nominal;
		ret->format.nBlockAlign = vi->channels;
		ret->format.wBitsPerSample = bytesPerSample * 8;
		ret->format.cbSize = 0;
		ret->head.lpData = (LPSTR)&ret[1];
		ret->head.dwBufferLength = comSize;
		ret->head.dwFlags = 0L;
		ret->head.dwLoops = 0L;
		memcpy(&ret[1], &buffer.front(), comSize);

		ov_clear( &vf );

		return ret;
	}
#endif

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

static wavc_t *root = NULL;

int playSound(const char *fileName, size_t delay, unsigned short vol, double pitch, signed char pan, unsigned short loops, char priority){
	wavc_t **node;
	if(vol < 16)
		return 0;
	for(node = &root; *node;){
		int res;
		res = strncmp((*node)->fname, fileName, sizeof (*node)->fname);
		if(res < 0) node = &(*node)->hi;
		else if(0 < res) node = &(*node)->lo;
		else break;
	}
	if(!*node){
		DWORD data;
		FILE *fp;

		if(!(*node = cacheWaveData(fileName))){
			return 0;
		}
		strncpy((*node)->fname, fileName, sizeof(*node)->fname);
		(*node)->hi = (*node)->lo = NULL;
	}

	SoundSource ss;
	ss.sampleBytes = (*node)->format.wBitsPerSample / 8;
	ss.channels = (*node)->format.nChannels;
	ss.src = (uint8_t*)(*node)->head.lpData;
	ss.size = (*node)->head.dwBufferLength / ss.sampleBytes / ss.channels;
	ss.delay = delay;
	ss.vol = vol;
	ss.pitch = pitch;
	ss.rate = (*node)->format.nSamplesPerSec;
	ss.pan = pan;
	ss.loops = loops;
	ss.priority = priority;
	return addsound(ss);
}

int playSound3DCustom(const char *fileName, const Vec3d &pos, size_t delay, double vol, double attn, double pitch, bool loop){
	wavc_t **node;
	if(!fileName) /* allow NULL pointer as file name */
		return 0;
	for(node = &root; *node;){
		int res;
		res = strncmp((*node)->fname, fileName, sizeof (*node)->fname);
		if(res < 0) node = &(*node)->hi;
		else if(0 < res) node = &(*node)->lo;
		else break;
	}
	if(!*node){
		DWORD data;
		FILE *fp;

		if(!(*node = cacheWaveData(fileName))){
			return 0;
		}
		strncpy((*node)->fname, fileName, sizeof(*node)->fname);
		(*node)->hi = (*node)->lo = NULL;
	}

	SoundSource3D ss;
	ss.sampleBytes = (*node)->format.wBitsPerSample / 8;
	ss.channels = (*node)->format.nChannels;
	ss.src = (decltype(ss.src))(*node)->head.lpData;
	ss.size = (*node)->head.dwBufferLength / ss.sampleBytes / ss.channels;
	ss.delay = delay;
	ss.vol = vol;
	ss.attn = attn;
	ss.pos = pos;
	ss.pitch = pitch;
	ss.rate = (*node)->format.nSamplesPerSec;
	ss.loop = loop;
	return addsound3d(ss);
}

int playSound3D(const char *fname, const Vec3d &src, double vol, double attn, double delay, bool loop, double pitch){
	extern double dwo;
	return playSound3DCustom(fname, src, delay < dwo ? 0 : (size_t)(SAMPLERATE * (delay - dwo)), vol, attn, pitch, loop);
}

int playMemoryWave3D(const unsigned char *wav, size_t size, double pitch, const double src[3], const double org[3], const double pyr[3], double vol, double attn, double delay){
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
	SoundSource3D ss;
	ss.sampleBytes = 1;
	ss.src = (decltype(ss.src))wav;
	ss.size = size / ss.sampleBytes;
	ss.delay = delay < dwo ? 0 : (size_t)(SAMPLERATE * (delay - dwo/* + dist / wave_sonic_speed*/));
	ss.vol = vol;
	ss.attn = attn;
	ss.pos = src;
	ss.pitch = pitch;
	ss.rate = 8000;
	ss.loop = false;
	return addsound3d(ss);
}

#else
/* I'm sorry I don't know how to play it */
int playWAVEFile(const char *fname){fname; return 0;}
int playWaveCustom(const char *fname, unsigned short, unsigned short, char pan){fname, pan; return 0;}
#endif

}
