#include "avi.h"
#include "cmd.h"
#include <windows.h>
#include <vfw.h> /* link with Vfw32.lib */
#include <GL/gl.h>

int g_avi_framerate = 30;

static PAVIFILE g_avi;
static PAVISTREAM g_as;
static long g_frame;

/*
static void WriteStream(PAVISTREAM lpas, LPAVISTREAMINFO lpasi)
{
	HDC              hdcMem;
	DWORD            i;
	TCHAR            szBuf[256];
	LPVOID           lpBits;
	HBITMAP          hbmpMem, hbmpMemPrev;
	BITMAPINFOHEADER bih = {0};

	bih.biSize      = sizeof(BITMAPINFOHEADER);
	bih.biWidth     = lpasi->rcFrame.right;
	bih.biHeight    = lpasi->rcFrame.bottom;
	bih.biPlanes    = 1;
	bih.biBitCount  = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = bih.biWidth * bih.biHeight * 3;

	AVIStreamSetFormat(lpas, 0, &bih, sizeof(BITMAPINFOHEADER));

	hdcMem  = CreateCompatibleDC(NULL);
	hbmpMem = CreateDIBSection(NULL, (LPBITMAPINFO)&bih, DIB_RGB_COLORS, &lpBits, NULL, 0);

	hbmpMemPrev = (HBITMAP)SelectObject(hdcMem, hbmpMem);
	SetBkMode(hdcMem, TRANSPARENT);

	for (i = 0; i < lpasi->dwLength; i++) {
		wsprintf(szBuf, TEXT("%d"), i);
		FillRect(hdcMem, &lpasi->rcFrame, (HBRUSH)GetStockObject(GRAY_BRUSH));
		DrawText(hdcMem, szBuf, -1, &lpasi->rcFrame, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		AVIStreamWrite(lpas, i, 1, lpBits, bih.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	}
	
	SelectObject(hdcMem, hbmpMemPrev);
	DeleteObject(hbmpMem);
	DeleteDC(hdcMem);
}*/


#define LENG 10
#define PIXELS 100
#define WIDTH 1024
#define HEIGHT 768
#define LINE ((((WIDTH)*24+31)&~31)/8)
#define SIZEIMAGE (LINE*(HEIGHT))
#define PI 3.1415926535

int video(int argc, char *argv[])
{
    int i,j,s;
    AVISTREAMINFO si={streamtypeVIDEO,comptypeDIB,0,0,0,0,
            1,30,0,LENG,0,0,(DWORD)-1,0,{0,0,WIDTH,HEIGHT},0,0,("Video #1")};
    BITMAPINFOHEADER bmih={sizeof(BITMAPINFOHEADER),WIDTH,HEIGHT,1,24,BI_RGB,
                                                            SIZEIMAGE,0,0,0,0};
    static BYTE bBit[SIZEIMAGE];
    PAVIFILE pavi;
    PAVISTREAM pstm;

	si.dwRate = g_avi_framerate;

    memset(bBit,0,SIZEIMAGE);
    AVIFileInit();

	if (AVIFileOpen(&pavi,(1 < argc ? argv[1] : "TEST01.AVI"),
                            OF_CREATE | OF_WRITE | OF_SHARE_DENY_NONE,NULL)!=0)
        return 0;
    if (AVIFileCreateStream(pavi,&pstm,&si)!=0)
        return 0;
    if (AVIStreamSetFormat(pstm,0,&bmih,sizeof(BITMAPINFOHEADER))!=0)
        return 0;

	i = 0;
/*    for (i=0;i<LENG;i++) {
        for (j=0;j<PIXELS;j++) {
            s=(int)(cos(PI*2*j/PIXELS)*min(WIDTH,HEIGHT)*(i*PIXELS+j)
                                            /(LENG*PIXELS*2)+WIDTH/2)*3
                +(int)(sin(PI*2*j/PIXELS)*min(WIDTH,HEIGHT)*(i*PIXELS+j)
                                            /(LENG*PIXELS*2)+HEIGHT/2)*LINE;
            bBit[s]=bBit[s+1]=bBit[s+2]=0xff;
        }
        if (AVIStreamWrite(pstm,i,1,bBit,SIZEIMAGE,
                                                AVIIF_KEYFRAME,NULL,NULL)!=0)
            return 0;
    }*/

	g_avi = pavi;
	g_as = pstm;
	g_frame = i;

	CmdPrint("video rec start");

 /*   AVIStreamRelease(pstm);
    AVIFileRelease(pavi);
    AVIFileExit();*/
    return 0;
}

int video_stop(int argc, char *argv[]){
	if(g_as){
		AVIStreamRelease(g_as);
		g_as = NULL;
	}
	
	if(g_avi){
		AVIFileRelease(g_avi);
		g_avi = NULL;
	}

	CmdPrint("video rec stop");

	return 0;
}

int video_frame(){
	if(g_as){
		static BYTE bBit[SIZEIMAGE];
		static DWORD src[WIDTH * HEIGHT];
		int i, j;
		glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, src);
		for(i = 0; i < HEIGHT; i++) for(j = 0; j < WIDTH; j++){
			DWORD dw = src[i * WIDTH + j];
			bBit[i * LINE + j * 3] = (dw >> 16) & 0xff;
			bBit[i * LINE + j * 3 + 1] = (dw >> 8) & 0xff;
			bBit[i * LINE + j * 3 + 2] = dw & 0xff;
		}
		AVIStreamWrite(g_as, g_frame++, 1, bBit, SIZEIMAGE, AVIIF_KEYFRAME, NULL, NULL);
		return 1;
	}
	return 0;
}
