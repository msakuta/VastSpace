#ifndef BITMAP_H
#define BITMAP_H
#include "export.h"
#include <windows.h>

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

typedef struct drawdata{
	HDC hdc;
	BITMAP bm;
	unsigned long t;
} drawdata_t;


void DrawBitmapPaletteTransparent(drawdata_t *dd, const BITMAPINFO *bi, const RGBQUAD *pal, int x0, int y0, const RECT *pr, unsigned tp);
void DrawBitmapTransparent(drawdata_t *dd, const BITMAPINFO *bi, int x0, int y0, const RECT *r, unsigned tp);
void DrawBitmapPaletteMask(drawdata_t *dd, const BITMAPINFO *image, const RGBQUAD *pal, int x0, int y0, const RECT *r, const BITMAPINFO *mask, unsigned maskindex);
void DrawBitmapMask(drawdata_t *dd, const BITMAPINFO *image, int x0, int y0, const RECT *r, const BITMAPINFO *mask, unsigned maskindex);


BITMAPINFO *LoadJpeg(const char *jpgfilename);

#ifdef __cplusplus
}
#endif

#endif
