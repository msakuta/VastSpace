// previewdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMainDlg;

class CPreviewDlg : public CWindowImpl<CPreviewDlg, CWindow, CWinTraits<WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE, 0> >
{
public:
	HDC hdc;
	HGLRC hgl;
	CMainDlg &main;
	double dist;
	Quatd viewrot;
	int boneview;
	int g_width, g_height, g_max;

	CPreviewDlg(CMainDlg &a) : hgl(NULL), main(a), dist(1000.), viewrot(quat_u), boneview(0)
	{}

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
//		CenterWindow(GetParent());
		hgl = wingl(m_hWnd, &hdc);
		return TRUE;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if(hgl){
			reshape_func(LOWORD(lParam), HIWORD(lParam));
		}
		return TRUE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
//		EndDialog(wID);
		return 0;
	}

	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT ret = DefWindowProc(uMsg, wParam, lParam);
/*		{
			CDC cdc = GetDC();
			cdc.TextOut(100, 100, _T("Hello, WTL!!"));
		}*/
			if(hgl){
				HDC hdc;
//				HGLRC hgl;
				hdc = GetDC();
//				hgl = winglstart(hdc);
				display_func();
				wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
//				winglend(hgl);
				ReleaseDC(hdc);
			}
		return ret;
	}

	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		static POINT lastcursor;
		if(wParam & MK_LBUTTON){
			long dx = LOWORD(lParam) - lastcursor.x;
			long dy = HIWORD(lParam) - lastcursor.y;

			aquat_t iviewrot;
			QUATCNJ(iviewrot, viewrot);
			Quat<double> qx;
			quatrot(qx, iviewrot, avec3_010);
/*				qx[0] = 0;
			qx[1] = sin(dx * M_PI / 2 / g_width);
			qx[2] = 0;*/
			VECSCALEIN(qx, sin(dx * M_PI / 2 / g_width));
			qx[3] = cos(dx * M_PI / 2 / g_width);
			aquat_t qr;
			QUATMUL(qr, viewrot, qx);
			QUATCPY(viewrot, qr);
			Quat<double> qy;
			quatrot(qy, iviewrot, avec3_100);
			((Vec3<double>&)qy).scalein(sin(dy * M_PI / 2 / g_height));
			qy[3] = cos(dy * M_PI / 2 / g_height);
			QUATMUL(qr, viewrot, qy);
			QUATCPY(viewrot, qr);
			Invalidate(0);
		}
		lastcursor.x = LOWORD(lParam);
		lastcursor.y = HIWORD(lParam);
		return 0;
	}

	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		mouse_func((short)HIWORD(wParam) < 0 ? GLUT_WHEEL_DOWN : GLUT_WHEEL_UP, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
		return 0;
	}

	void reshape_func(int w, int h)
	{
		int m = w < h ? h : w;
		glViewport(0, 0, w, h);
		g_width = w;
		g_height = h;
		g_max = m;

	#if 0

	#elif 1
	  /* 変換行列の初期化 */
	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();

	  /* スクリーン上の表示領域をビューポートの大きさに比例させる */
	/*  glOrtho(-w / 200.0, w / 200.0, -h / 200.0, h / 200.0, -1.0, 1.0);*/

	  gluPerspective(30.0, (double)w / (double)h, .10, 100000.0);
	  glTranslated(0.0, 0.0, -5.0);
	   glMatrixMode(GL_MODELVIEW);
	 
	#else

	  /* 透視変換行列の設定 */
	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
	 /* glTranslated(0.0, 0.0, -50.0);*/
	  gluPerspective(30.0, (double)w / (double)h, 1.0, 100.0);

	  /* モデルビュー変換行列の設定 */
	  glMatrixMode(GL_MODELVIEW);
	#endif
	}

	static HGLRC wingl(HWND hWnd, HDC *phdc){
		PIXELFORMATDESCRIPTOR pfd = { 
			sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
			1,                     // version number 
			PFD_DRAW_TO_WINDOW |   // support window 
			PFD_SUPPORT_OPENGL |   // support OpenGL 
			PFD_DOUBLEBUFFER,      // double buffered 
			PFD_TYPE_RGBA,         // RGBA type 
			24,                    // 24-bit color depth 
			0, 0, 0, 0, 0, 0,      // color bits ignored 
			0,                     // no alpha buffer 
			0,                     // shift bit ignored 
			0,                     // no accumulation buffer 
			0, 0, 0, 0,            // accum bits ignored 
			32,                    // 32-bit z-buffer 
			0,                     // no stencil buffer 
			0,                     // no auxiliary buffer 
			PFD_MAIN_PLANE,        // main layer 
			0,                     // reserved 
			0, 0, 0                // layer masks ignored 
		}; 
		int  iPixelFormat; 
		HGLRC hgl;
		HDC hdc;

		hdc = ::GetDC(hWnd);

		// get the best available match of pixel format for the device context  
		iPixelFormat = ChoosePixelFormat(hdc, &pfd); 
			
		// make that the pixel format of the device context 
		SetPixelFormat(hdc, iPixelFormat, &pfd);

		hgl = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hgl);

		*phdc = hdc;

		return hgl;
	}

	static HGLRC winglstart(HDC hdc){
		PIXELFORMATDESCRIPTOR pfd = { 
			sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
			1,                     // version number 
			PFD_DRAW_TO_WINDOW |   // support window 
			PFD_SUPPORT_OPENGL |   // support OpenGL 
			PFD_DOUBLEBUFFER,      // double buffered 
			PFD_TYPE_RGBA,         // RGBA type 
			24,                    // 24-bit color depth 
			0, 0, 0, 0, 0, 0,      // color bits ignored 
			0,                     // no alpha buffer 
			0,                     // shift bit ignored 
			0,                     // no accumulation buffer 
			0, 0, 0, 0,            // accum bits ignored 
			32,                    // 32-bit z-buffer 
			0,                     // no stencil buffer 
			0,                     // no auxiliary buffer 
			PFD_MAIN_PLANE,        // main layer 
			0,                     // reserved 
			0, 0, 0                // layer masks ignored 
		}; 
		int  iPixelFormat; 
		HGLRC hgl;

		// get the best available match of pixel format for the device context  
		iPixelFormat = ChoosePixelFormat(hdc, &pfd); 
			
		// make that the pixel format of the device context 
		SetPixelFormat(hdc, iPixelFormat, &pfd);

		hgl = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hgl);

		return hgl;
	}

	void mouse_func(int button, int state, int x, int y){
		if(button == GLUT_WHEEL_UP)
			dist /= 1.1;
		else if(button == GLUT_WHEEL_DOWN)
			dist *= 1.1;
		Invalidate(0);
		return;
		aquat_t qx;
		qx[0] = 0;
		qx[1] = sin((x - g_width / 2) * M_PI / 2 / g_width);
		qx[2] = 0;
		qx[3] = cos((x - g_width / 2) * M_PI / 2 / g_width);
		aquat_t qr;
		QUATMUL(qr, viewrot, qx);
		QUATCPY(viewrot, qr);
		aquat_t qy = {
			sin((y - g_height / 2) * M_PI / 2 / g_height),
			0,
			0,
			cos((y - g_height / 2) * M_PI / 2 / g_height),
		};
		QUATMUL(qr, viewrot, qy);
		QUATCPY(viewrot, qr);
	}

	void display_func(void);

	const MotionNode *FindBone(int boneid);
	const char *GetSrfName(int boneid);
	void bone_ysdnm_node(ysdnm_t *dnm, struct ysdnm_srf *srf, ysdnmv_t *v0, const Mat4d &pmat);
	void BoneYSDNM_V(ysdnm_t *dnm, ysdnmv_t *dnmv);
};
