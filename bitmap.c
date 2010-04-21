#include "bitmap.h"
#include <clib/zip/UnZip.h>
#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static void smemcpy(void *a, const void *b, size_t s){
	for(; s; s--){
		*((char*)a)++ = *((const char*)b)++;
	}
}

#define memcpy smemcpy

BITMAPINFO *ReadBitmap(const char *szFileName)
{
	HWND hWnd = NULL;
    DWORD dwResult;
	DWORD dwFileSize, dwSizeImage;
    HANDLE hF;
    BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
	char *szBuffer;

    hF = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hF == INVALID_HANDLE_VALUE) {
        goto tryzip;
    }
    ReadFile(hF, &bf, sizeof(BITMAPFILEHEADER), &dwResult, NULL);
    
	dwFileSize = bf.bfSize;

	if (/*memcmp(&bf.bfType, "BM", 2) != 0*/ bf.bfType != *(WORD*)"BM") {
		CloseHandle(hF);
		goto tryzip;
	}

    ReadFile(hF, &bi, sizeof(BITMAPINFOHEADER), &dwResult, NULL);

	dwSizeImage = bf.bfSize - bf.bfOffBits;

    szBuffer = LocalAlloc(LMEM_FIXED, dwFileSize - sizeof(BITMAPFILEHEADER));

    SetFilePointer(hF, sizeof(BITMAPFILEHEADER), 0, FILE_BEGIN);
    ReadFile(hF, szBuffer,
        dwSizeImage + bi.biClrUsed * sizeof(RGBQUAD) + sizeof(BITMAPINFOHEADER),
        &dwResult, NULL);
    
    CloseHandle(hF);
    return (BITMAPINFO*)szBuffer;

tryzip:
	{
		char *pv;
		pv = ZipUnZip("rc.zip", szFileName, &dwFileSize);
		if(!pv)
			return NULL;
		szBuffer = LocalAlloc(LMEM_FIXED, dwFileSize - sizeof(BITMAPFILEHEADER));
		memcpy(szBuffer, &pv[sizeof(BITMAPFILEHEADER)], dwFileSize - sizeof(BITMAPFILEHEADER));
		ZipFree(pv);
		return (BITMAPINFO*)szBuffer;
	}
}

void DrawBitmapPaletteTransparent(drawdata_t *dd, const BITMAPINFO *bi, const RGBQUAD *pal, int x0, int y0, const RECT *pr, unsigned tp){
	int dx, ssx;
	RECT r;
	unsigned char *head;
	if(dd->bm.bmBitsPixel != 32 || !dd->bm.bmBits || bi->bmiHeader.biCompression != BI_RGB)
		return;
	if(pr)
		r = *pr;
	else{
		r.left = r.top = 0;
		r.right = bi->bmiHeader.biWidth;
		r.bottom = bi->bmiHeader.biHeight;
	}

	/* mirrored image is specified by setting r.left greater than r.right.
	  in that case, we must tweak the values a little. */
	dx = r.right - r.left;
	ssx = dx < 0 ? -1 : 1;
	dx *= ssx; /* absolute */

	/* in a mirrored image, x0 remains left edge coord, while sx scans opposite direction */
	if(x0 + dx < 0 || dd->bm.bmWidth <= x0 || y0 + (r.bottom - r.top) < 0 || dd->bm.bmWidth <= y0)
		return;

	head = &((unsigned char*)bi)[offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD)];

	switch(bi->bmiHeader.biBitCount){
	case 1:
	{
		int x, y, sx, sy;
/*		printf("2 col bpl: %d\n", (bi->bmiHeader.biWidth + 31) / 32 * 4);*/
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = head + (bi->bmiHeader.biWidth + 31) / 32 * 4 * (bi->bmiHeader.biHeight - sy - 1);
/*			printf("	%p\n", buf);*/
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				int index = 0x1 & (buf[sx / 8] >> (7 - (sx) % 8));
				if(index == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
	break;
	case 4:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
/*			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + (bi->bmiHeader.biWidth + 15) / 16 * 8 * (bi->bmiHeader.biHeight - sy - 1);*/
			unsigned char *buf = head + (bi->bmiHeader.biWidth * bi->bmiHeader.biBitCount + 31) / 32 * 4 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				int index = 0xf & (buf[sx / 2] >> ((sx+1) % 2 * 4));
				if(index == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
	break;
	case 8:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = head + (bi->bmiHeader.biWidth + 3) / 4 * 4 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				int index = buf[sx];
				if(index == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
	break;
	case 24:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + bi->bmiHeader.biWidth * 3 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				RGBQUAD q;
				CopyMemory(&q, &buf[sx * 3], 3);
				q.rgbReserved = 0;
				if(*(unsigned*)&q == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = q;
			}
		}
	}
	break;
	case 32:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + bi->bmiHeader.biWidth * 4 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				RGBQUAD *p = &((RGBQUAD*)buf)[sx];
				if(*(unsigned*)p == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = *p;
			}
		}
	}
	break;
	}
	return;
}

void DrawBitmapTransparent(drawdata_t *dd, const BITMAPINFO *bi, int x0, int y0, const RECT *r, unsigned tp){
	DrawBitmapPaletteTransparent(dd, bi, bi->bmiColors, x0, y0, r, tp);
}

void DrawBitmapPaletteMask(drawdata_t *dd, const BITMAPINFO *bi, const RGBQUAD *pal, int x0, int y0, const RECT *pr, const BITMAPINFO *mask, unsigned maskindex){
	RECT r;
	if(dd->bm.bmBitsPixel != 32 || !dd->bm.bmBits || bi->bmiHeader.biCompression != BI_RGB)
		return;
	if(pr)
		r = *pr;
	else{
		r.left = r.top = 0;
		r.right = bi->bmiHeader.biWidth;
		r.bottom = bi->bmiHeader.biHeight;
	}
	if(x0 + (r.right - r.left) < 0 || dd->bm.bmWidth <= x0 || y0 + (r.bottom - r.top) < 0 || dd->bm.bmWidth <= y0)
		return;
	if(bi->bmiHeader.biBitCount == 4 && mask->bmiHeader.biBitCount == 1){
		int x, y, sx, sy, mx, my;
		for(y = MAX(y0, 0), sy = y - y0 + r.top, my = sy - r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++, my++){
			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + (bi->bmiHeader.biWidth + 1) / 2 * (bi->bmiHeader.biHeight - sy - 1);
			unsigned char *mbuf = (unsigned char*)mask + offsetof(BITMAPINFO, bmiColors) + mask->bmiHeader.biClrUsed * sizeof(RGBQUAD) + (mask->bmiHeader.biWidth + 7) / 8 * (mask->bmiHeader.biHeight - my - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left, mx = sx - r.left; x <= MIN(x0 + r.right - r.left - 1, dd->bm.bmWidth-1); sx++, x++, mx++){
				int index = 0xf & (buf[sx / 2] >> ((sx+1) % 2 * 4));
				if(((mbuf[mx / 8] >> (7 - mx % 8)) & 1) == maskindex)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
}

void DrawBitmapMask(drawdata_t *dd, const BITMAPINFO *bi, int x0, int y0, const RECT *pr, const BITMAPINFO *mask, unsigned maskindex){
	DrawBitmapPaletteMask(dd, bi, bi->bmiColors, x0, y0, pr, mask, maskindex);
}










struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

static void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

BITMAPINFO *LoadJpeg(const char *jpgfilename){
	BITMAPINFO *bmi;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	FILE * infile;		/* source file */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in image buffer */
	int src_row_stride;

	if((infile = fopen(jpgfilename, "rb")) == NULL){
		fprintf(stderr, "can't open %s\n", jpgfilename);
		return NULL;
	}
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void) jpeg_read_header(&cinfo, TRUE);
	(void) jpeg_start_decompress(&cinfo);
	row_stride = cinfo.output_width * 3/*cinfo.output_components*/;
	src_row_stride = cinfo.output_width * cinfo.output_components;
	bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + cinfo.output_width * cinfo.output_height * 3/*cinfo.output_components*/);
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = cinfo.output_width; 
	bmi->bmiHeader.biHeight = -(LONG)(cinfo.output_height);
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 24;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = cinfo.output_width * cinfo.output_height * 3/*cinfo.output_components*/;
	bmi->bmiHeader.biXPelsPerMeter = 0;
	bmi->bmiHeader.biYPelsPerMeter = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, src_row_stride, 1);
	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned j;
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
//					memcpy(&((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride], buffer[0], row_stride);
		if(cinfo.output_components == 3) for(j = 0; j < cinfo.output_width; j++){
			JSAMPLE *dst = &((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride + j * cinfo.output_components];
			JSAMPLE *src = &buffer[0][j * cinfo.output_components];
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
		}
		else if(cinfo.output_components == 1) for(j = 0; j < cinfo.output_width; j++){
			JSAMPLE *dst = &((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride + j * 3];
			JSAMPLE *src = &buffer[0][j * cinfo.output_components];
			dst[0] = src[0];
			dst[1] = src[0];
			dst[2] = src[0];
		}
	}
	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	return bmi;
}
