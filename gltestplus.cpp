
#define USEWIN 1 /* whether use windows api (wgl*) */

#include "viewer.h"
#include "player.h"
#include "entity.h"
#include "coordsys.h"
#include "stellar_file.h"
#include "astrodraw.h"
#include "cmd.h"
#include "keybind.h"
#include "motion.h"

extern "C"{
#include <clib/timemeas.h>
#include <clib/c.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>


#if !USEWIN
#include <GL/glut.h>
#else
#include "antiglut.h"
#include <windows.h>
#endif
#include <GL/gl.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#define exit something_meanless
/*#include <windows.h>*/
#undef exit
#include <GL/glext.h>

static double g_fix_dt = 0.;
static double gametimescale = 1.;
static double g_space_near_clip = 0.01, g_space_far_clip = 1e3;
bool mouse_captured = false;
double gravityfactor = 1.;

CoordSys galaxysystem;

Player pl;

class Ball : public Entity{
public:
	typedef Entity st;
	double rad;
	Ball() : rad(1) {}
	Ball(const Vec3d &pos, const Vec3d &velo) : rad(1){}
	Ball(const Vec3d &pos, const Vec3d &velo, const Vec3d &omg) : rad(1){}
	void anim(double dt);
	void collide(Ball &o){
		Vec3d delta = (pos - o.pos);
		Vec3d n = delta.norm();
		double cs = (velo - o.velo).sp(delta); // Closing Speed
		velo -= delta * cs * 1;
		o.velo += delta * cs * 1;
	}
};

Ball balls[10];

void Ball::anim(double dt){
	pos += velo * dt;
/*	amat4_t mat;
	amat3_t omgt, nmat3;*/
	Quatd qomg, q, qbackup;

	qomg = omg.scale(dt / 2.);
	q = qomg * rot;
	VEC4ADD(rot, rot, q);
	rot.normin();
/*	if(nmat){
		QUATCPY(qbackup, pt->rot);
		QUATCPY(pt->rot, *pq);
		tankrot(*nmat, pt);
		VECSADD(&(*nmat)[12], pt->velo, dt);
		QUATCPY(pt->rot, qbackup);
	}*/
	if(pos[0] < -10){
		pos[0] = -10;
		velo[0] = velo[0] < 0 ? -velo[0] : 0;
	}
	if(10 < pos[0]){
		pos[0] = 10;
		velo[0] = 0 < velo[0] ? -velo[0] : 0;
	}
	if(pos[2] < -10){
		pos[2] = -10;
		velo[2] = velo[2] < 0 ? -velo[2] : 0;
	}
	if(10 < pos[2]){
		pos[2] = 10;
		velo[2] = 0 < velo[2] ? -velo[2] : 0;
	}
	int i;
	for(i = 0; i < numof(balls); i++) if(&balls[i] != this && (pos - balls[i].pos).slen() < 1 * 1 && (pos - balls[i].pos).sp(velo - balls[i].velo) < 0.){
		collide(balls[i]);
	}
}

void drawShadeSphere(){
	int n = 32, slices, stacks;
	double (*cuts)[2];
	cuts = CircleCuts(n);
	glBegin(GL_QUADS);
	for(slices = 0; slices < n / 2; slices++) for(stacks = 0; stacks < n; stacks++){
		int stacks1 = (stacks+1) % n, slices1 = (slices+1) % n;
		int m;
		void (WINAPI *glproc[2])(GLdouble, GLdouble, GLdouble) = {glNormal3d, glVertex3d};
		for(m = 0; m < 2; m++) glproc[m](cuts[stacks][0] * cuts[slices][0], cuts[stacks][1], cuts[stacks][0] * cuts[slices][1]);
		for(m = 0; m < 2; m++) glproc[m](cuts[stacks1][0] * cuts[slices][0], cuts[stacks1][1], cuts[stacks1][0] * cuts[slices][1]);
		for(m = 0; m < 2; m++) glproc[m](cuts[stacks1][0] * cuts[slices1][0], cuts[stacks1][1], cuts[stacks1][0] * cuts[slices1][1]);
		for(m = 0; m < 2; m++) glproc[m](cuts[stacks][0] * cuts[slices1][0], cuts[stacks][1], cuts[stacks][0] * cuts[slices1][1]);
	}
	glEnd();
}

void lightOn(){
	GLfloat light_pos[4] = {1, 2, 1, 0};
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

static void cslist(const CoordSys *root, double &y){
	char buf[256];
	buf[0] = '\0';
	if(root->getpath(buf, sizeof buf)){
//		glRasterPos3d(0, y, -1);
		glPushMatrix();
		glTranslated(0, y, -1);
		gldPutTextureString(buf);
		glPopMatrix();
	}
	CoordSys *cs;
	for(cs = root->children; cs; cs = cs->next){
		cslist(cs, --y);
	}
}

void draw_func(Viewer &vw, double dt){
	int i;
	for(i = 0; i < numof(balls); i++){
		balls[i].anim(dt);
	}
	glClearDepth(1.);
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	glMultMatrixd(vw.rot);
	gldTranslaten3dv(vw.pos);
	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glColor4f(1,1,1,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawstarback(&vw, &galaxysystem, NULL, NULL);
	glPopAttrib();
	glTranslated(0, -10, -10);
	for(i = -8; i <= 8; i++){
		glBegin(GL_LINES);
		glVertex3d(i, -1., -8.);
		glVertex3d(i, -1., 8.);
		glVertex3d(-8., -1., i);
		glVertex3d(8., -1., i);
		glEnd();
	}
	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	lightOn();
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
	glVertex3d(-8, -1, -8);
	glVertex3d(-8, -1,  8);
	glVertex3d( 8, -1,  8);
	glVertex3d( 8, -1, -8);
	glEnd();
	for(i = 0; i < numof(balls); i++){
		glPushMatrix();
		gldTranslate3dv(balls[i].pos);
		gldMultQuat(balls[i].rot);
		drawShadeSphere();
		glPopMatrix();
	}
	glPopAttrib();
	glPopMatrix();

	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glColor4f(1,1,1,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glScaled(1. / (vw.vp.m / 16.), 1. / (vw.vp.m / 16.), 1.);
	double y = vw.vp.h / 16.;
	cslist(&galaxysystem, y);
	glPopAttrib();
	glPopMatrix();
}

static POINT mouse_pos = {0, 0};

void display_func(void){
	static int init = 0;
	static timemeas_t tm;
	static double gametime = 0.;
	double dt = 0.;
	if(!init){
		extern double dwo;
		init = 1;
/*
#ifdef _WIN32
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
		glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
		glMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC)wglGetProcAddress("glMultiTexCoord1fARB");
#endif
*/
//		anim_sun(0.);
		TimeMeasStart(&tm);
//		warf.soundtime = TimeMeasLap(&tmwo) - dwo;
	}
	else{
		int trainride = 0;
		double t1, rdt;

		t1 = TimeMeasLap(&tm);
		if(g_fix_dt)
			rdt = g_fix_dt;
		else
			rdt = (t1 - gametime) * gametimescale;

		dt = !init ? 0. : rdt < 1. ? rdt : 1.;

		if(mouse_captured){
			POINT p;
			if(GetCursorPos(&p) && (p.x != mouse_pos.x || p.y != mouse_pos.y)){
				aquat_t q;
//				quatirot(q, pl.rot, vec3_010);
				VECCPY(q, vec3_010);
				VECSCALEIN(q, -(p.x - mouse_pos.x) * .001 / 2.);
				q[3] = 0.;
				quatrotquat(pl.rot, q, pl.rot);
//				quatirot(q, pl.rot, vec3_100);
				VECCPY(q, vec3_100);
				VECSCALEIN(q, -(p.y - mouse_pos.y) * .001 / 2.);
				q[3] = 0.;
				quatrotquat(pl.rot, q, pl.rot);
				SetCursorPos(mouse_pos.x, mouse_pos.y);
//				mouse_pos = p;
			}
		}

		MotionAnim(pl, dt);

		pl.pos += pl.velo * dt;

		galaxysystem.anim(dt);

		gametime = t1;
	}
	Viewer viewer;
	{
		double hack[16] = {
			1,0,0,0,
			0,1,0,0,
			0,0,10,0,
			0,0,0,1,
		};
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		viewer.vp.set(vp);
		double dnear = g_space_near_clip, dfar = g_space_far_clip;
/*		if(pl.cs->w && pl.cs->w->vft->nearplane)
			dnear = pl.cs->w->vft->nearplane(pl.cs->w);
		if(pl.cs->w && pl.cs->w->vft->farplane)
			dfar = pl.cs->w->vft->farplane(pl.cs->w);*/
		glMatrixMode (GL_PROJECTION);    /* prepare for and then */ 
		glLoadIdentity();               /* define the projection */
		viewer.frustum(dnear, dfar);  /* transformation */
/*		glMultMatrixd(hack);*/
		glGetDoublev(GL_PROJECTION_MATRIX, hack);
		glMatrixMode (GL_MODELVIEW);  /* back to modelview matrix */
/*		glDepthRange(.5,100);*/
	}
	viewer.cs = pl.cs = &galaxysystem;
	viewer.pos = pl.pos;
	quat2mat(viewer.rot, pl.rot);
	quat2imat(viewer.irot, pl.rot);
	viewer.relrot = viewer.rot;
	viewer.relirot = viewer.irot;
	draw_func(viewer, dt);
}

void mouse_func(int button, int state, int x, int y){

/*	if(cmdwnd){
		CmdMouseInput(button, state, x, y);
		return;
	}*/

/*	if(!mouse_captured){
		if(glwdrag){
			if(state == GLUT_UP)
				glwdrag = NULL;
			return;
		}
		else{
			int killfocus = 1, ret = 0;
			ret = glwMouseFunc(button, state, x, y);
			if(!ret){
				if(!glwfocus && button == GLUT_LEFT_BUTTON && state == GLUT_UP){
					avec3_t centerray, centerray0;
					aquat_t qrot, qirot;
					centerray0[0] = (s_mousedragx + s_mousex) / 2. / gvp.w - .5;
					centerray0[1] = (s_mousedragy + s_mousey) / 2. / gvp.h - .5;
					centerray0[2] = -1.;
					VECSCALEIN(centerray0, -1.);
					QUATCNJ(qrot, pl.rot);
					quatrot(centerray, qrot, centerray0);
					quatdirection(qrot, centerray);
					QUATCNJ(qirot, qrot);
					select_box(fabs(s_mousedragx - s_mousex), fabs(s_mousedragx - s_mousex), qirot);
					s_mousedragx = s_mousex;
					s_mousedragy = s_mousey;
				}
				glwfocus = NULL;
			}
			if(ret)
				return;
		}
	}

	if(0 && pl.control){
		if(state == GLUT_UP && button == GLUT_WHEEL_UP)
			s_mouse |= PL_MWU;
		if(state == GLUT_UP && button == GLUT_WHEEL_DOWN)
			s_mouse |= PL_MWD;
	}
	else{
		extern double g_viewdist;
		if(state == GLUT_UP && button == GLUT_WHEEL_UP)
			g_viewdist *= 1.2;
		if(state == GLUT_UP && button == GLUT_WHEEL_DOWN)
			g_viewdist /= 1.2;
	}

	if(!state){
		s_mouse |= !state << (button / 2);
	}
	else
		s_mouse &= ~(!!state << (button / 2));*/
/*	s_mousec |= 1 << (button / 2);*/
/*
	s_mousex = x;
	s_mousey = y;

	{
		int i = (y - 40) / 10;
		if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && gvp.w - 160 <= x && 0 <= i && i < OV_COUNT){
			if(g_counts[i] == 1){
				pl.chase = pl.selected = g_tanks[i];
			}
			else
				pl.selected = NULL;
			current_vft = g_vfts[i];
		}
	}
*/
#if USEWIN && defined _WIN32
	if(/*!cmdwnd &&*/ (!pl.control || !mouse_captured) && state == GLUT_UP && button == GLUT_RIGHT_BUTTON){
		mouse_captured = !mouse_captured;
/*		printf("right up %d\n", mouse_captured);*/
		if(mouse_captured){
			int c;
			HWND hwd;
			RECT r;
			hwd = GetActiveWindow();
			GetClientRect(hwd, &r);
			mouse_pos.x = (r.left + r.right) / 2;
			mouse_pos.y = (r.top + r.bottom) / 2;
			ClientToScreen(hwd, &mouse_pos);
			SetCursorPos(mouse_pos.x, mouse_pos.y);
			c = ShowCursor(FALSE);
			while(0 <= c)
				c = ShowCursor(FALSE);
//			glwfocus = NULL;
		}
		else{
//			s_mousedragx = s_mousex;
//			s_mousedragy = s_mousey;
			while(ShowCursor(TRUE) < 0);
		}
	}
#endif
}

void reshape_func(int w, int h)
{
	int m = w < h ? h : w;
	glViewport(0, 0, w, h);
//	g_width = w;
//	g_height = h;
//	g_max = m;
}

extern "C" int console_cursorposdisp;
int console_cursorposdisp = 0;

#if USEWIN && defined _WIN32
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

	hdc = GetDC(hWnd);

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

static void winglend(HGLRC hgl){
	wglMakeCurrent (NULL, NULL) ;
	if(!wglDeleteContext(hgl))
		fprintf(stderr, "eeee!\n");
}
#endif

#define DELETEKEY 0x7f
#define ESC 0x1b

static void key_func(unsigned char key, int x, int y){
	BindExec(key);
}

static LRESULT WINAPI CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	static HDC hdc;
	static HGLRC hgl = NULL;
	static int moc = 0;
	switch(message){
		case WM_CREATE:
			hgl = wingl(hWnd, &hdc);
			if(!SetTimer(hWnd, 2, 10, NULL))
				assert(0);
			break;

		case WM_TIMER:
			if(hgl){
				HDC hdc;
//				HGLRC hgl;
				hdc = GetDC(hWnd);
//				hgl = winglstart(hdc);
				display_func();
				wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
//				winglend(hgl);
				ReleaseDC(hWnd, hdc);
			}
			break;

		case WM_SIZE:
			if(hgl){
				reshape_func(LOWORD(lParam), HIWORD(lParam));
			}
			break;

		case WM_RBUTTONDOWN:
			mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONDOWN:
			mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_RBUTTONUP:
			mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONUP:
			mouse_func(GLUT_LEFT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_CHAR:
			key_func(wParam, 0, 0);
			break;

		/* technique to enable momentary key commands */
		case WM_KEYUP:
			BindKeyUp(toupper(wParam));
			switch(wParam){
			case VK_DELETE: BindKeyUp(DELETEKEY); break;
//			case VK_ESCAPE: BindKeyUp(ESC); break;
			case VK_ESCAPE: if(mouse_captured){
				mouse_captured = false;
				while(ShowCursor(TRUE) < 0);
			}
//			case VK_SHIFT: BindKeyUp(1); break;
//			case VK_CONTROL: BindKeyUp(2); break;
			}
			break;

		/* TODO: don't call every time the window defocus */
		case WM_KILLFOCUS:
			BindKillFocus();
			break;

		case WM_DESTROY:
			KillTimer(hWnd, 2);

			if(moc){
				ReleaseCapture();
				while(ShowCursor(TRUE) < 0);
				ClipCursor(NULL);
			}

			PostQuitMessage(0);
			break;
		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


#if defined _WIN32
HWND hWndApp;
#endif

int main(int argc, char *argv[])
{
	int i;

	viewport vp;
	CmdInit(&vp);
	MotionInit();
	CmdAdd("bind", cmd_bind);
	CmdAdd("pushbind", cmd_pushbind);
	CmdAdd("popbind", cmd_popbind);
	CmdExec("@exec autoexec.cfg");

	StellarFileLoad("space.dat", &galaxysystem);

	{
		random_sequence rs;
		init_rseq(&rs, 342925);
		for(i = 0; i < numof(balls); i++){
			double x = drseq(&rs) * 20 - 10, z = drseq(&rs) * 20 - 10;
			Vec3d pos = Vec3d(x, 0, z);
			x = drseq(&rs) * 20 - 10, z = drseq(&rs) * 20 - 10;
			Vec3d velo = Vec3d(x, 0, z);
			x = (drseq(&rs) * 2 - 1) * M_PI, z = (drseq(&rs) * 2 - 1) * M_PI;
			Vec3d omg = Vec3d(x, z, (drseq(&rs) * 2 - 1) * M_PI);
			balls[i] = Ball(pos, velo, omg);
		}
	}

#if USEWIN
	{
		MSG msg;
		HWND hWnd;
		ATOM atom;
		HINSTANCE hInst;
		RECT rc = {100,100,100+512,100+384};
/*		RECT rc = {0,0,0+512,0+384};*/
		hInst = GetModuleHandle(NULL);

		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX); 

			wcex.style			= CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc	= (WNDPROC)WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= hInst;
			wcex.hIcon			= LoadIcon(hInst, IDI_APPLICATION);
			wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground	= NULL;
			wcex.lpszMenuName	= NULL;
			wcex.lpszClassName	= "gltest";
			wcex.hIconSm		= LoadIcon(hInst, IDI_APPLICATION);

			atom = RegisterClassEx(&wcex);
		}


		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CAPTION, FALSE);

		hWndApp = hWnd = CreateWindow(LPCTSTR(atom), "gltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
/*		hWnd = CreateWindow(atom, "gltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			100, 100, 400, 400, NULL, NULL, hInst, NULL);*/

		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
#else
/*	printf("%lg %lg %lg\n", fmod(-8., 5.), -8. / 5. - (int)(-8. / 5.), -8. / 5. - (floor)(-8. / 5.));*/
/*	display_func();*/
	glutMainLoop();
#endif

	while(ShowCursor(TRUE) < 0);
	while(0 <= ShowCursor(FALSE));

	return 0;
}

#if 0 && defined _WIN32

HINSTANCE g_hInstance;

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, int nCmdShow) 
{
	g_hInstance = hinstance;
	return main(1, lpCmdLine);
}
#endif
