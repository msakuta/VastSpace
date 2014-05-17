#include <clib/zip/UnZip.h>
#include <clib/c.h>
#include <zlib.h>
#include <stdio.h>
#include <string.h>
#ifndef NDEBUG
#include <clib/timemeas.h>
#endif

#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
#else
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
#endif

#ifdef _WIN32
#pragma pack(push, 2)
#endif

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

typedef struct sZipLocalHeader{
	uint32_t signature; //    local file header signature     4 bytes  (0x04034b50)
	uint16_t version; // version needed to extract       2 bytes
	uint16_t flag; //general purpose bit flag        2 bytes
	uint16_t method; //compression method              2 bytes
	uint16_t modtime; //last mod file time              2 bytes
	uint16_t moddate; //last mod file date              2 bytes
	uint32_t crc32; //crc-32                          4 bytes
	uint32_t csize; //compressed size                 4 bytes
	uint32_t usize; //uncompressed size               4 bytes
	uint16_t namelen; //file name length                2 bytes
	uint16_t extralen; //extra field length              2 bytes
} PACKED ZipLocalHeader;

typedef struct sZipCentralDirHeader
{
	uint32_t signature;
	uint16_t madever;
	uint16_t needver;
	uint16_t option;
	uint16_t comptype;
	uint16_t filetime;
	uint16_t filedate;
	uint32_t crc32;
	uint32_t compsize;
	uint32_t uncompsize;
	uint16_t fnamelen;
	uint16_t extralen;
	uint16_t commentlen;
	uint16_t disknum;
	uint16_t inattr;
	uint32_t outattr;
	uint32_t headerpos;
} PACKED ZipCentralDirHeader;

typedef struct sZipEndCentDirHeader
{
	uint32_t signature;
	uint16_t disknum;
	uint16_t startdisknum;
	uint16_t diskdirentry;
	uint16_t direntry;
	uint32_t dirsize;
	uint32_t startpos;
	uint16_t commentlen;
} PACKED ZipEndCentDirHeader;

#ifdef _WIN32
#pragma pack(pop, 2)
#endif


static int pathncmp (const char *first, const char *last, unsigned count)
{
	register char cfirst, clast;
	if (!count)
		return(0);
	while (--count && *first && (*first == '\\' ? '/' : *first) == (*last == '\\' ? '/' : *last)){
		first++;
		last++;
	}
	return((*first == '\\' ? '/' : *first) - (*last == '\\' ? '/' : *last));
}

typedef struct sMPOS{
	unsigned char *org;
	unsigned char *cur;
	unsigned char *end;
#ifdef NDEBUG
	const char *fname; // debugging feature
#endif
} MPOS;

static size_t mread(void *buf, size_t s, size_t c, MPOS *fp){
	size_t ret = MIN((fp->end - fp->cur), s * c) / s;
	memcpy(buf, s * ret, fp->cur);
	return ret;
}

static int mseek(MPOS *fp, long v, int whence){
	switch(whence){
		case SEEK_SET: fp->cur = &((char*)fp->org)[v]; break;
		case SEEK_CUR: fp->cur += v; break;
		case SEEK_END: fp->cur = fp->end[v]; break;
	}
}

void *ZipUnZipV(int fromMemory, FILE *fp, const char *fname, const char *ename, unsigned long *psize){
	int reti, isdir = 0;
	void *ret = NULL, *cret;
	ZipLocalHeader lh;
	ZipCentralDirHeader cdh;
	ZipEndCentDirHeader ecd;
	char namebuf[256];
	unsigned long usize, cmplen;
	z_stream z;
	size_t (*pread)(void *, size_t, size_t, FILE*) = fromMemory ? mread : fread;
	int (*pseek)(FILE *, long, int) = fromMemory ? mseek : fseek;
#ifndef NDEBUG
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	pseek(fp, -(long)sizeof ecd, SEEK_END);
	pread(&ecd, sizeof ecd, 1, fp);
	if(ecd.signature != 0x06054B50)
		goto error_tag;
	pseek(fp, ecd.startpos, SEEK_SET);

	while(pread(&cdh, sizeof cdh, 1, fp)){
		if(cdh.signature != 0x02014b50 || sizeof namebuf <= cdh.fnamelen)
			goto error_tag;
		pread(namebuf, cdh.fnamelen, 1, fp);
		namebuf[cdh.fnamelen] = '\0'; /* Null terminate to avoid partial matching */
		pseek(fp, cdh.extralen, SEEK_CUR); /* skip extra field */
		if(ename && (!cdh.fnamelen || pathncmp(ename, namebuf, isdir ? cmplen : cdh.fnamelen + 1))){
			continue;
		}
		pseek(fp, cdh.headerpos, SEEK_SET);
		pread(&lh, sizeof lh, 1, fp);

		/* Raw format can be read without any conversion */
		if(lh.method == 0){
			ret = malloc(lh.csize);
			pseek(fp, lh.namelen + lh.extralen, SEEK_CUR);
			pread(ret, lh.csize, 1, fp);
			if(psize)
				*psize = lh.csize;
#ifndef NDEBUG
			fprintf(stderr, "%s(%s) %lu/%lu=%lg%% %lgs\n", fname, ename, lh.csize, lh.usize, (double)100. * lh.csize / lh.usize, TimeMeasLap(&tm));
#endif
			return ret;
		}

		/* Bail if the compression algorithm is not deflate */
		if(lh.method != 8)
			return NULL;

		pseek(fp, lh.namelen + lh.extralen, SEEK_CUR);
		cret = malloc(lh.csize);
		pread(cret, lh.csize, 1, fp);
		if(isdir){
			MPOS mp;
			mp.org = cret;
			mp.cur = cret;
			mp.end = &mp.org[lh.csize];
			ret = ZipUnZipV(1, &mp, fname, &ename[cmplen+1], psize);
			free(cret);
			return ret;
		}
		else{
			ret = malloc(lh.usize);
			usize = lh.usize;
			z.zalloc = Z_NULL;
			z.zfree = Z_NULL;
			z.opaque = Z_NULL;
			z.next_in = Z_NULL;
			z.avail_in = 0;
			inflateInit2(&z, -15);
			z.next_in = cret;
			z.avail_in = lh.csize;
			z.next_out = ret;
			z.avail_out = usize;
			reti = inflate(&z, Z_NO_FLUSH);
			inflateEnd(&z);
			free(cret);
			if(psize)
				*psize = usize;
		}
#ifndef NDEBUG
		fprintf(stderr, "%s(%s) %lu/%lu=%lg%% %lgs\n", fname, ename, lh.csize, lh.usize, (double)100. * lh.csize / lh.usize, TimeMeasLap(&tm));
#endif
		break;
	}
error_tag:
//	fclose(fp);
	return ret;
}

void *ZipUnZip(const char *fname, const char *ename, unsigned long *psize){
	FILE *fp;
	void *ret;
	fp = fopen(fname, "rb");
	if(!fp)
		return NULL;
	ret = ZipUnZipV(0, fp, fname, ename, psize);
	fclose(fp);
	return ret;
}

void ZipFree(void *pv){
	free(pv);
}
