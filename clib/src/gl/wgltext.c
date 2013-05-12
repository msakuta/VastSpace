#include "clib/gl/gltext.h"
#include <windows.h>
#include <GL/gl.h>

void gldDrawTextW(const wchar_t *s){
	BITMAP bm;
	static int init = 0;
	static HDC hcdc;
	static HBITMAP hbm;
	static GLubyte *chbuf;

	if(!init){
		init = 1;
		{
			HDC hwdc;
			HWND hw;
			struct BITMAPINFO2{
				BITMAPINFOHEADER biHeader;
				RGBQUAD biColors[2];
			} bi = {{
				sizeof (struct BITMAPINFO2), //DWORD  biSize; 
				32, //LONG   biWidth; 
				32, //LONG   biHeight; 
				1, //WORD   biPlanes; 
				1, //WORD   biBitCount; 
				BI_RGB, //DWORD  biCompression; 
				0, //DWORD  biSizeImage; 
				1000, //LONG   biXPelsPerMeter; 
				1000, //LONG   biYPelsPerMeter; 
				2, //DWORD  biClrUsed; 
				2, //DWORD  biClrImportant; 
			}, {0,0,0,0,255,255,255,0}};
			hw = GetDesktopWindow();
			hwdc = GetDC(hw);
			hcdc = CreateCompatibleDC(hwdc);
			hbm = CreateDIBSection(hcdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&chbuf, NULL, 0);
/*			int i;
			wchar_t buf[4] = L"‚Ÿ";*/
			SelectObject(hcdc, hbm);
			SelectObject(hcdc, GetStockObject(SYSTEM_FONT));
			SetBkColor(hcdc, RGB(255,255,255));
			SetTextColor(hcdc, RGB(0,0,0));
/*			for(i = 0; i < 100; i++){
				TextOutW(hcdc, 0, 0, buf, 1);
				glNewList(0x3041+i, GL_COMPILE);
				glBitmap(32, 32, 0, 0, 16, 0, chbuf);
				glEndList();
				(*buf)++;
			}
			DeleteObject(hbm);
			DeleteDC(hcdc);*/
			ReleaseDC(hw, hwdc);
		}
	}
	GetObject(hbm, sizeof bm, &bm);
//	printf("%p %p\n", bm.bmBits, chbuf);
/*	SelectObject(hcdc, hbm);
	SelectObject(hcdc, GetStockObject(DEFAULT_GUI_FONT));
	SetBkColor(hcdc, RGB(255,255,255));
	SetTextColor(hcdc, RGB(0,0,0));*/
/*	GLfloat rp[4];
	glGetFloatv(GL_CURRENT_RASTER_POSITION, rp);*/

	while(*s != L'\0'){
//		ZeroMemory(chbuf, 16 * 16 / 4);
		int w;
//		WORD ww;
//		char aa;
//		DWORD cbuf[1] = {0};
//		ABC abc;
		GCP_RESULTSW gr = {sizeof(GCP_RESULTSW)};
//		wctomb((char*)cbuf, *s);
//		GetGlyphIndicesW(hcdc, s, 1, &ww, GGI_MARK_NONEXISTING_GLYPHS);
//		GetCharWidthI(hcdc, ww, 1, NULL, &w);
//		GetCharABCWidths(hcdc, *cbuf, *cbuf, &abc);
//		gr.lpCaretPos = &w;
//		gr.lpClass = &aa;
		gr.nGlyphs = 1;
		gr.nMaxFit = 1;
		w = GetCharacterPlacementW(hcdc, s, 1, 1000, &gr, 0);
		w &= 0xffff;
		if(0 && 0 <= *s - 0x3041 && *s - 0x3041 < 100)
			glCallList(*s);
		else{
			RECT r = {0, 0, 32, 32};
//			int di[2];
			ExtTextOutW(hcdc, 0, 0, ETO_OPAQUE, &r, s, 1, NULL);
/*			glRecti(rp[0], rp[1], rp[0] + w, rp[1] + 20);
			rp[0] += w;*/
			glBitmap(32, 32, 0, 0, (GLfloat)w, 0, chbuf);
		}
		s++;
	}
}
