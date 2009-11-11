
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
#include <clib/cfloat.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/gl/cullplus.h>


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


#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);

static double g_fix_dt = 0.;
static double gametimescale = 1.;
static double g_space_near_clip = 0.01, g_space_far_clip = 1e3;
bool mouse_captured = false;
int gl_wireframe = 0;
double gravityfactor = 1.;
int g_gear_toggle_mode = 0;
double flypower = 1.;
static int show_planets_name = 1;
static int cmdwnd = 0;

PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLMULTITEXCOORD2DARBPROC glMultiTexCoord2dARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;

Player pl;
Universe galaxysystem(&pl);
const char *Universe::classname()const{
	return "Universe";
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
	const Astrobj *sun = pl.cs->findBrightest(pl.pos);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, sun ? Vec4<GLfloat>(pl.cs->tocs(vec3_000, sun).normin().cast<GLfloat>()) : light_pos);
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

static void draw_gear(double dt){
	double (*cuts)[2];
	double desired;
	int i;
	static double gearphase = 0;

	desired = pl.gear * 360 / 8;
	if(gearphase == desired && !((g_gear_toggle_mode ? MotionGetToggle : MotionGet)() & PL_G))
		return;
	if(gearphase != desired)
		gearphase = approach(gearphase, desired, 360 * dt, 360);

	cuts = CircleCuts(8 * 6);

	glPushMatrix();
	glRotated((-gearphase), 0, 0, -1.);
	glBegin(GL_LINE_LOOP);
	for(i = 0; i < 8 * 6; i++){
		double f = (i + 4) % 6 < 3 ? .5 : .7;
		glVertex2d(f * cuts[i][0], f * cuts[i][1]);
	}
	glEnd();
	glPopMatrix();

	glBegin(GL_LINE_LOOP);
	glVertex2d(.15, .75);
	glVertex2d(-.15, .75);
	glVertex2d(-.15, .45);
	glVertex2d(.15, .45);
	glEnd();

	for(i = 0; i < 8; i++){
		glPushMatrix();
		glRotated(i * 360 / 8 - gearphase, 0, 0, -1.);
		glTranslated(0, 0.6, 0);
		glScaled(.05, .05, 1.);
		gldPolyChar(i + '1');
		glPopMatrix();
	}
}

static void drawastro(Viewer *vw, CoordSys *cs, const Mat4d &model){
	int id = 0;
	OrbitCS *a = cs->toOrbitCS();
	do if(a){
		Vec3d pos, wpos;
		pos = vw->cs->tocs(a->pos, a->parent);
		mat4vp3(wpos, model, pos);
		if(a->orbit_home && a->flags2 & OCS_SHOWORBIT){
			int j;
			double (*cuts)[2], rad;
			const Astrobj *home = a->orbit_home;
			Vec3d spos;
			Mat4d mat, qmat, rmat, lmat;
			Quatd q;
			double smia; /* semi-minor axis */
			cuts = CircleCuts(64);
			spos = vw->cs->tocs(home->pos, home->parent);
			mat = vw->cs->tocsim(home->parent);
			qmat = a->orbit_axis.tomat4();
			rad = a->orbit_rad;
			smia = rad * (a->eccentricity != 0 ? sqrt(1. - a->eccentricity * a->eccentricity) : 1.);
			qmat.scalein(rad, smia, rad);
			if(a->eccentricity != 0)
				qmat.translatein(a->eccentricity, 0, 0);
			rmat = mat * qmat;
/*				VECSUB(&rmat[12], spos, vw->pos);*/
		/*	projection*/(glPushMatrix());
/*				glLoadIdentity ();
			glFrustum (g_left * -0.0005, g_right * 0.0005,
				g_bottom * -0.0005, g_top * 0.0005,
				g_near * 0.0005, g_far * 1e10); */
			glLoadMatrixd(vw->rot);
			glBegin(GL_LINE_LOOP);
			for(j = 0; j < 64; j++){
				avec3_t v, vr;
				v[0] = 0 + cuts[j][0];
				v[1] = 0 + cuts[j][1];
				v[2] = 0;
				mat4vp3(vr, rmat, v);
				VECADDIN(vr, spos);
				if(VECSDIST(vr, vw->pos) < .15 * .15 * rad * rad){
					int k;
					for(k = 0; k < 8; k++){
						v[0] = 0 + sin(2 * M_PI * (j * 8 + k) / (64 * 8));
						v[1] = 0 + cos(2 * M_PI * (j * 8 + k) / (64 * 8));
						v[2] = 0;
						mat4vp3(vr, rmat, v);
						VECADDIN(vr, spos);
						VECSUBIN(vr, vw->pos);
						VECNORMIN(vr);
						glVertex3dv(vr);
					}
				}
				else{
					VECSUBIN(vr, vw->pos);
					VECNORMIN(vr);
					glVertex3dv(vr);
				}
/*					glVertex3d(spos[0] + SUN_DISTANCE * cuts[j][0], spos[1] + SUN_DISTANCE * cuts[j][1], spos[2]);*/
			}
			glEnd();
		/*	projection*/(glPopMatrix());
		}
#if 1
		double rad = a->toAstrobj() ? a->toAstrobj()->rad : a->csrad;
		if(0. < wpos[2] || !(a->flags & AO_ALWAYSSHOWNAME) && rad / -wpos[2] < .00001
			/*&& !(a->flags & AO_PLANET && a->orbit_home && astrobj_visible(vw, a->orbit_home)*/)
			break;
#endif
		pos[0] = wpos[0] / wpos[2];
		pos[1] = wpos[1] / wpos[2];
		glPushMatrix();
		glLoadIdentity();
		glTranslated(-pos[0], -pos[1], -1.);
		{
			double r = 5. / vw->vp.m * vw->fov;
			glBegin(GL_LINE_LOOP);
			glVertex2d(r, r);
			glVertex2d(-r, r);
			glVertex2d(-r, -r);
			glVertex2d(r, -r);
			glEnd();
		}
		glBegin(GL_LINES);
		glVertex2d(0., 0.);
		glVertex2d(.05, .05 - id * .01);
		glEnd();
		glRasterPos2d(0.05, 0.05 - id * .01);
		gldprintf("%s", a->name);
		glPopMatrix();
	} while(0);
	for(CoordSys *cs2 = cs->children; cs2; cs2 = cs2->next)
		drawastro(vw, cs2, model);
}

static void drawindics(Viewer *vw){
	if(show_planets_name){
		Mat4d model;
		model = vw->rot;
		MAT4TRANSLATE(model, -vw->pos[0], -vw->pos[1], -vw->pos[2]);
		drawastro(vw, &galaxysystem, model);
//		drawCSOrbit(vw, &galaxysystem);
	}
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0,0,-1);
	glRasterPos2d(-(double)vw->vp.w / vw->vp.m, -(double)vw->vp.h / vw->vp.m);
	gldprintf("%s %s", pl.cs->classname(), pl.cs->name);
	glPopMatrix();
}

void draw_func(Viewer &vw, double dt){
	glClearDepth(1.);
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	glMultMatrixd(vw.rot);
	GLcull glc = GLcull(vw.fov, vw.pos, vw.irot, 1. / 1e3, 1e10);
	vw.gc = &glc;
	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, gl_wireframe ? GL_LINE : GL_FILL);
	glDisable(GL_CULL_FACE);
	glColor4f(1,1,1,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	drawstarback(&vw, &galaxysystem, NULL, NULL);
	projection((
		glPushMatrix(), glLoadIdentity(),
		vw.frustum(1. / 1e3, 1e10)
	));
	galaxysystem.startdraw();
	galaxysystem.predraw(&vw);
	galaxysystem.drawcs(&vw);
	projection(glPopMatrix());
	glPopAttrib();

	gldTranslaten3dv(vw.pos);
	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	lightOn();
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	if(pl.cs->w)
		pl.cs->w->draw(NULL);
	glPopAttrib();
	glPopMatrix();

	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glColor4ub(255, 255, 255, 255);
	drawindics(&vw);
	glLoadIdentity();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glColor4f(1,1,1,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glScaled(1. / (vw.vp.m / 16.), 1. / (vw.vp.m / 16.), 1.);
	double y = vw.vp.h / 16.;
	cslist(&galaxysystem, y);
	glPopMatrix();

	glPushMatrix();
	glLoadIdentity();
	glTranslated(0,0,-1);
	draw_gear(dt);
	glPopAttrib();
	glPopMatrix();

	if(cmdwnd)
		CmdDraw(vw.vp);
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

#define proc(t,a) a = (t)wglGetProcAddress(#a)
		proc(PFNGLACTIVETEXTUREARBPROC, glActiveTextureARB);
		proc(PFNGLMULTITEXCOORD2DARBPROC, glMultiTexCoord2dARB);
		proc(PFNGLMULTITEXCOORD2FARBPROC, glMultiTexCoord2fARB);
		proc(PFNGLMULTITEXCOORD1FARBPROC, glMultiTexCoord1fARB);
#undef proc

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
				double speed = .001 / 2. * pl.fov;
				aquat_t q;
//				quatirot(q, pl.rot, vec3_010);
				VECCPY(q, vec3_010);
				VECSCALEIN(q, (p.x - mouse_pos.x) * speed);
				q[3] = 0.;
				quatrotquat(pl.rot, q, pl.rot);
//				quatirot(q, pl.rot, vec3_100);
				VECCPY(q, vec3_100);
				VECSCALEIN(q, (p.y - mouse_pos.y) * speed);
				q[3] = 0.;
				quatrotquat(pl.rot, q, pl.rot);
				SetCursorPos(mouse_pos.x, mouse_pos.y);
//				mouse_pos = p;
			}
		}

		Vec3d pos;
		const CoordSys *cs = pl.cs->belongcs(pos, pl.pos);
		if(cs != pl.cs){
			pl.cs = cs;
			pl.pos = pos;
		}

		MotionFrame(dt);

		MotionAnim(pl, dt, flypower);

		if(pl.chase){
			input_t inputs;
			inputs.press = MotionGet();
			inputs.change = MotionGetToggle();
			pl.chase->control(&inputs, dt);
			pl.pos = pl.chase->pos;
			pl.velo = pl.chase->velo;
		}
		pl.velolen = pl.velo.len();
		pl.pos += pl.velo * (1. + pl.velolen) * dt;

		galaxysystem.anim(dt);
		galaxysystem.endframe();

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
		viewer.fov = pl.fov;
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
	viewer.cs = pl.cs;
	viewer.pos = pl.pos;
	if(pl.chase){
		Quatd rot = pl.rot * pl.chase->rot.cnj();
		viewer.rot = rot.tomat4();
		viewer.irot = rot.cnj().tomat4();
		viewer.pos = pl.pos + pl.chase->rot.trans(Vec3d(.0, .05, .15));
	}
	else{
		viewer.rot = pl.rot.tomat4();
		viewer.irot = pl.rot.cnj().tomat4();
	}
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

static int cmd_teleport(int argc, char *argv[]){
	const char *arg = argv[1];
	if(!arg){
		CmdPrint("Specify location you want to teleport to.");
		return 0;
	}
	if(pl.chase){
		return 0;
	}
	{
		int i;
		for(i = 0; i < ntplist; i++) if(!strcmp(argv[1], tplist[i].name)){
			pl.cs = tplist[i].cs;
			VECCPY(pl.pos, tplist[i].pos);
			VECNULL(pl.velo);
			break;
		}
		if(i == ntplist)
			CmdPrintf("Could not find location \"%s\".", arg);
	}
	return 0;
}

static int cmd_eject(int argc, char *argv[]){
	if(pl.chase){
		pl.chase = NULL;
		pl.pos += pl.rot.cnj() * Vec3d(0,0,.3);
	}
	return 0;
}


/*
extern "C" int console_cursorposdisp;
int console_cursorposdisp = 0;*/

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

static void special_func(int key, int x, int y){
	if(cmdwnd){
		CmdSpecialInput(key);
		return;
	}
/*	if(glwfocus && glwfocus->key){
		int ret;
		glwindow **referrer;
		ret = glwfocus->key(glwfocus, key);
		if(glwfocus->flags & GLW_TODELETE) for(referrer = &glwlist; *referrer; referrer = &(*referrer)->next) if(*referrer == glwfocus){
			*referrer = glwfocus->next;
			free(glwfocus);
			glwfocus = NULL;
			break;
		}
		if(ret)
			return;
	}*/
	BindExecSpecial(key);
}

static void key_func(unsigned char key, int x, int y){
	if(cmdwnd){
		if(/*key == DELETEKEY ||*/ key == ESC){
			cmdwnd = 0;
			return;
		}
		if(key == 1 || key == 2)
			return;
		if(!CmdInput(key))
			return;
	}

	switch(key){
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			if((g_gear_toggle_mode ? MotionGetToggle : MotionGet)() & PL_G){
				pl.gear = key - '1';
				MotionSetToggle(PL_G, 0);
				flypower = .05 * pow(16, double(key - '2'));

/*				if(warp_move == pl.mover && key != '9'){
					flypower = .05 * pow(16, key - '2');
					break;
				}*/

/*				if(key == '1'){
//					pl.mover = player_walk;
					pl.gear = 0;
					flypower = .05 * pow(16, key - '2');
				}
				else if(key == '9'){
//					pl.mover = warp_move;
					flypower = .05 * pow(16, key - '2');
				}
				else{
//					pl.mover = add_velo_win;
					flypower = .05 * pow(16, key - '2');
				}*/
			}
			break;
	}
	BindExec(key);
}

static void non_printable_key(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, int state){
	switch(wParam){
	case VK_DELETE: key_func(DELETEKEY, 0, 0); break;
	case VK_ESCAPE: key_func(ESC, 0, 0); break;
	case VK_PRIOR: special_func(GLUT_KEY_PAGE_UP, 0, 0); break;
	case VK_NEXT: special_func(GLUT_KEY_PAGE_DOWN, 0, 0); break;
	case VK_HOME: special_func(GLUT_KEY_HOME, 0, 0); break;
	case VK_END: special_func(GLUT_KEY_END, 0, 0); break;
	case VK_INSERT: special_func(GLUT_KEY_INSERT, 0, 0); break;
	case VK_LEFT: special_func(GLUT_KEY_LEFT, 0, 0); break;
	case VK_RIGHT: special_func(GLUT_KEY_RIGHT, 0, 0); break;
	case VK_UP: special_func(GLUT_KEY_UP, 0, 0); break;
	case VK_DOWN: special_func(GLUT_KEY_DOWN, 0, 0); break;
	case VK_SHIFT: key_func(1, 0, 0); break;
	case VK_CONTROL: key_func(2, 0, 0); break;
	case VK_MENU: key_func(3, 0, 0); break;
/*			case VK_BACK: key_func(0x08, 0, 0);*/
	}
	if(VK_F1 <= wParam && wParam <= VK_F12)
		special_func(GLUT_KEY_F1 + wParam - VK_F1, 0, 0);
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

		/* most inputs from keyboard is processed through WM_CHAR, but some special keys are
		  not sent as characters. */
		case WM_KEYDOWN:
			if(VK_NUMPAD0 <= wParam && wParam <= VK_NUMPAD9)
				BindExec(wParam - VK_NUMPAD0 + '\010');
			else
				non_printable_key(hWnd, message, wParam, lParam, 0);
			break;

		case WM_SYSKEYDOWN:
			non_printable_key(hWnd, message, wParam, lParam, 0);
			break;

		/* technique to enable momentary key commands */
		case WM_KEYUP:
			if(VK_NUMPAD0 <= wParam && wParam <= VK_NUMPAD9)
				BindKeyUp(wParam - VK_NUMPAD0 + '\010');
			else
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

		case WM_SYSKEYUP:
			BindKeyUp(wParam == VK_MENU ? 3 : toupper(wParam));
			switch(wParam){
			case VK_DELETE: BindKeyUp(DELETEKEY); break;
			case VK_ESCAPE: BindKeyUp(ESC); break;
			case VK_SHIFT: BindKeyUp(1); break;
			case VK_CONTROL: BindKeyUp(2); break;
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

static int cmd_toggleconsole(int argc, char *argv[]){
	cmdwnd = !cmdwnd;
	if(cmdwnd){
		mouse_captured = 0;
/*		while(ShowCursor(TRUE) <= 0);*/
	}
	return 0;
}


#if defined _WIN32
HWND hWndApp;
#endif

int main(int argc, char *argv[])
{

	viewport vp;
	CmdInit(&vp);
	MotionInit();
	CmdAdd("bind", cmd_bind);
	CmdAdd("pushbind", cmd_pushbind);
	CmdAdd("popbind", cmd_popbind);
	CmdAdd("toggleconsole", cmd_toggleconsole);
	CmdAdd("teleport", cmd_teleport);
	CmdAdd("eject", cmd_eject);
	CvarAdd("gl_wireframe", &gl_wireframe, cvar_int);
	CvarAdd("g_gear_toggle_mode", &g_gear_toggle_mode, cvar_int);
	CvarAdd("g_drawastrofig", &show_planets_name, cvar_int);
	CmdExec("@exec autoexec.cfg");

	StellarFileLoad("space.dat", &galaxysystem);

	pl.cs = &galaxysystem;

#if USEWIN
	{
		MSG msg;
		HWND hWnd;
		ATOM atom;
		HINSTANCE hInst;
		RECT rc = {100,100,100+1024,100+768};
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
			wcex.lpszClassName	= "gltestplus";
			wcex.hIconSm		= LoadIcon(hInst, IDI_APPLICATION);

			atom = RegisterClassEx(&wcex);
		}


		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CAPTION, FALSE);

		hWndApp = hWnd = CreateWindow(LPCTSTR(atom), "gltestplus", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
