#ifndef BITMAP_H
#define BITMAP_H
#include "export.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <stdint.h>

#define BI_RGB 0

typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint16_t WORD;
typedef uint16_t BYTE;

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;

#endif

#ifdef __cplusplus
extern "C"{
#endif

/* if it's not compressed, biCompression may not be trusty. */
#define sizeofbmp(bi) (((bi)->bmiHeader.biCompression == BI_RGB ?\
		((bi)->bmiHeader.biWidth * (bi)->bmiHeader.biBitCount + 31) / 32 * 4 * (bi)->bmiHeader.biHeight : (bi)->bmiHeader.biSizeImage)\
		+ offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD))

EXPORT BITMAPINFO *ReadBitmap(const char *szFileName);
EXPORT BITMAPINFO *ReadJpeg(const char *fileName, void (**freeproc)(BITMAPINFO*));
EXPORT BITMAPINFO *ReadPNG(const char *, void (**freeproc)(BITMAPINFO*));







// Following is obsolete functions

#ifdef _WIN32
typedef struct drawdata{
	HDC hdc;
	BITMAP bm;
	unsigned long t;
} drawdata_t;


void DrawBitmapPaletteTransparent(drawdata_t *dd, const BITMAPINFO *bi, const RGBQUAD *pal, int x0, int y0, const RECT *pr, unsigned tp);
void DrawBitmapTransparent(drawdata_t *dd, const BITMAPINFO *bi, int x0, int y0, const RECT *r, unsigned tp);
void DrawBitmapPaletteMask(drawdata_t *dd, const BITMAPINFO *image, const RGBQUAD *pal, int x0, int y0, const RECT *r, const BITMAPINFO *mask, unsigned maskindex);
void DrawBitmapMask(drawdata_t *dd, const BITMAPINFO *image, int x0, int y0, const RECT *r, const BITMAPINFO *mask, unsigned maskindex);
#endif

BITMAPINFO *LoadJpeg(const char *jpgfilename);

#ifdef __cplusplus
}
#endif

#endif
