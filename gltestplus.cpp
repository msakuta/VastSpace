/** \file 
 * \brief The main source file for gltestplus project.
 *
 * The entry point for the program, no matter main() or WinMain(), resides in this file.
 * It deals with the main message loop, OpenGL initialization, and event mapping.
 * Some drawing methods are also defined here.
 */

#define USEWIN 1 ///< whether use windows api (wgl*)

#if !USEWIN
#include <GL/glut.h>
#else
#include "antiglut.h"
#include "Client.h"
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include "Game.h"
#include "Viewer.h"
#include "Player.h"
#include "Entity.h"
#include "CoordSys.h"
#include "stellar_file.h"
#include "astrodraw.h"
#include "cmd.h"
#include "keybind.h"
#include "motion.h"
#include "glw/glwindow.h"
#include "glw/GLWmenu.h"
#include "Docker.h"
#include "draw/material.h"
#include "Sceptor.h"
#include "glstack.h"
#include "Universe.h"
#include "glsl.h"
#include "sqadapt.h"
#include "EntityCommand.h"
#include "astro_star.h"
#include "serial_util.h"
#include "draw/WarDraw.h"
#include "draw/ShadowMap.h"
#include "draw/ShaderBind.h"
#include "draw/OpenGLState.h"
#include "avi.h"

extern "C"{
#include <clib/timemeas.h>
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/gl/fbo.h>
#include <clib/suf/sufdraw.h>
#include <clib/zip/UnZip.h>
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/gl/cullplus.h>


#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#define exit something_meanless
/*#include <windows.h>*/
#undef exit
//#include <mmsystem.h>
#include <GL/glext.h>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>


#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);
#define texturemat(e) glMatrixMode(GL_TEXTURE); e; glMatrixMode(GL_MODELVIEW);

static double g_fix_dt = 0.;
static double gametimescale = 1.;
static double g_background_near_clip = 1e-10, g_background_far_clip = 1e10; ///< For those that z buffering does not make sense
static double g_space_near_clip = .5, g_space_far_clip = 1e10;
static double g_warspace_near_clip = 0.001, g_warspace_far_clip = 1e3;
static double r_dynamic_range = 1.;
static int r_orbit_axis = 0;
static int r_shadows = 1;
static bool mouse_captured = false;
static bool mouse_tracking = false;
int gl_wireframe = 0;
double gravityfactor = 1.;
int g_gear_toggle_mode = 0;
static int show_planets_name = 0;
static int cmdwnd = 0;
//static bool g_focusset = false;
//GLwindow *glwcmdmenu = NULL;

int s_mousex, s_mousey;
static int s_mousedragx, s_mousedragy;
static int s_mouseoldx, s_mouseoldy;

static double wdtime = 0., watime = 0.;

#if defined _WIN32
HWND hWndApp;
#endif

class select_box_callback{
public:
	virtual void onEntity(Entity *) = 0; ///< Callback for all Entities in the selection box.
};

static bool select_box(double x0, double x1, double y0, double y1, const Mat4d &rot, unsigned flags, select_box_callback *sbc);
//static void war_draw(Viewer &vw, const CoordSys *cs, void (WarField::*method)(wardraw_t *wd), GLuint tod = 0);
class WarDrawInt : public WarDraw{
	Viewer &gvw;
	Player *&player;
	const CoordSys *cs;
	void (WarField::*method)(wardraw_t *wd);
	void war_draw_int();
public:
	WarDrawInt(Viewer &vw, Player *&player, const CoordSys *cs, void (WarField::*method)(wardraw_t *wd), ShadowMap *sm = NULL);
	void draw();
	~WarDrawInt();
	WarDrawInt &setViewer(Viewer *vw){this->vw = vw; return *this;}
	WarDrawInt &shadowmap(bool b = true){shadowmapping = b; return *this;}
	WarDrawInt &setShader(GLuint a, GLint textureLoc, GLint shadowmapLoc){shader = a; this->textureLoc = textureLoc; this->shadowmapLoc = shadowmapLoc; return *this;}
};

static void war_draw(Viewer &vw, Player *&player, const CoordSys *cs, void (WarField::*method)(WarDraw *)){
	WarDrawInt(vw, player, cs, method).setViewer(&vw).draw();
}

ServerGame *server; ///< The server game.
Game *client; ///< The client game.


extern GLuint screentex;
GLuint screentex = 0;


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

static Vec3d g_light;

void Game::lightOn(){
	GLfloat light_pos[4] = {1, 2, 1, 0};
	const Astrobj *sun = player->cs->findBrightest(player->getpos());
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	g_light = sun ? player->cs->tocs(vec3_000, sun).normin() : vec3_010;
	glLightfv(GL_LIGHT0, GL_POSITION, sun ? Vec4<GLfloat>(g_light.cast<GLfloat>()) : light_pos);
}

void Game::draw_gear(double dt){
	double (*cuts)[2];
	double desired;
	int i;
	static double gearphase = 0;

	desired = player->freelook->getGear() * 360 / 8;
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
	OrbitCS *a = cs->toOrbitCS();
	do if(a){
		Vec3d pos = vw->cs->tocs(a->pos, a->parent);
		Vec3d wpos = model.vp3(pos);

		typedef std::map<const CoordSys *, std::vector<dstring> > LineMap;
		extern std::map<const CoordSys *, std::vector<dstring> > linemap;
		LineMap::iterator it = linemap.find(cs);
		if(it != linemap.end()){
			std::vector<dstring> &vec = it->second;
			glPushMatrix();
			glLoadMatrixd(vw->rot);
			glBegin(GL_LINES);
			for(int i = 0; i < vec.size(); i++){
				CoordSys *cs2 = cs->findcspath(vec[i]);
				if(cs2){
					glVertex3dv((pos - vw->pos).normin());
					glVertex3dv((vw->cs->tocs(Vec3d(0,0,0), cs2) - vw->pos).normin());
				}
			}
			glEnd();
			glPopMatrix();
		}

		if(a->toAstrobj() && 0. < a->omg.slen()){
			Vec3d omg = vw->cs->tocs(a->omg, a->parent, true).norm();
			glPushMatrix();
			glLoadMatrixd(vw->rot);
			glBegin(GL_LINES);
			glVertex3dv(pos + omg * a->toAstrobj()->rad - vw->pos);
			glVertex3dv(pos - omg * a->toAstrobj()->rad - vw->pos);
			glEnd();
			glPopMatrix();
		}

		bool parentBarycenter = a->orbit_center;
		if((a->orbit_home || parentBarycenter) && a->flags2 & OCS_SHOWORBIT){
			int j;
			double (*cuts)[2], rad;
			const CoordSys *home = parentBarycenter ? a->orbit_center : a->orbit_home;
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

			// Axis lines
			if(r_orbit_axis){
				glBegin(GL_LINES);
				glVertex3dv((rmat.vp3(Vec3d(-1, 0, 0)) + spos - vw->pos).normin());
				glVertex3dv((rmat.vp3(Vec3d( 1.2, 0, 0)) + spos - vw->pos).normin());
				glVertex3dv((rmat.vp3(Vec3d(0, -1, 0)) + spos - vw->pos).normin());
				glVertex3dv((rmat.vp3(Vec3d(0,  1.2, 0)) + spos - vw->pos).normin());
				glEnd();
			}

			// Ellipse
			glBegin(GL_LINE_LOOP);
			for(j = 0; j < 64; j++){
				Vec3d v(cuts[j][0], cuts[j][1], 0);
				Vec3d vr = rmat.vp3(v);
				vr += spos;
				if((vr - vw->pos).slen() < .15 * .15 * rad * rad){
					int k;
					for(k = 0; k < 8; k++){
						v = Vec3d(sin(2 * M_PI * (j * 8 + k) / (64 * 8)),
								  cos(2 * M_PI * (j * 8 + k) / (64 * 8)), 0);
						vr = rmat.vp3(v);
						vr += spos;
						vr -= vw->pos;
						vr.normin();
						glVertex3dv(vr);
					}
				}
				else{
					vr -= vw->pos;
					vr.normin();
					glVertex3dv(vr);
				}
/*					glVertex3d(spos[0] + SUN_DISTANCE * cuts[j][0], spos[1] + SUN_DISTANCE * cuts[j][1], spos[2]);*/
			}
			glEnd();
		/*	projection*/(glPopMatrix());
		}
#if 1
		double rad = a->toAstrobj() ? a->toAstrobj()->rad : a->csrad;
		bool isStar = false;
		if((0. <= wpos[2] || !(isStar = &a->getStatic() == &Star::classRegister) && !(a->flags & AO_ALWAYSSHOWNAME) && rad / -wpos[2] < .00001)
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
		// Draw custom overlay defined in script file.
		do{
			HSQUIRRELVM v = g_sqvm;
			StackReserver sr(v);
			sq_pushroottable(v);
			sq_pushstring(v, _SC("drawCoordSysLabel"), -1);
			if(SQ_FAILED(sq_get(v, -2)))
				break;
			sq_pushroottable(v);
			sq_pushstring(v, cs->getStatic().s_sqclassname, -1);
			if(SQ_FAILED(sq_get(v, -2)))
				break;
			if(SQ_FAILED(sq_createinstance(v, -1)))
				break;
			sqa_newobj(v, cs, -1);
			sq_remove(v, -2);
			sq_call(v, 2, SQTrue, SQTrue);
			const SQChar *s;
			if(SQ_FAILED(sq_getstring(v, -1, &s)))
				break;
			glBegin(GL_LINES);
			glVertex2d(0., 0.);
			glVertex2d(.05 * vw->fov, .05 * vw->fov);
			glEnd();
			glTranslated(0.05 * vw->fov, 0.05 * vw->fov, 0);
			glScaled(2. / vw->vp.m * vw->fov, -2. / vw->vp.m * vw->fov, 1.);
			glwPutTextureString(s);
		}while(0);

		glPopMatrix();
	} while(0);
	for(CoordSys *cs2 = cs->children; cs2; cs2 = cs2->next)
		drawastro(vw, cs2, model);
}


static double diprint(const char *s, double x, double y){
	glPushMatrix();
	glTranslated(x, y, 0.);
	glScaled(8. * 1, -8. * 1, 1.);
	gldPutTextureString(s);
	glPopMatrix();
	return x + strlen(s) * 8;
}

extern int bullet_kills, missile_kills;
int bullet_kills = 0, missile_kills = 0, wire_kills = 0;
int bullet_shoots = 0, bullet_hits = 0;

void Game::drawindics(Viewer *vw){
	viewport &gvp = vw->vp;
	if(show_planets_name){
		Mat4d model;
		model = vw->rot;
		model.translatein(-vw->pos[0], -vw->pos[1], -vw->pos[2]);
		GLpmatrix();
//		projection(vw->frustum(g_space_near_clip, g_space_far_clip));
		drawastro(vw, universe, model);
//		drawCSOrbit(vw, &galaxysystem);
	}
	if(player->chase){
		wardraw_t wd;
		wd.vw = vw;
		wd.w = *player->chase->w;
		if(wd.w)
			player->chase->drawHUD(&wd);
	}
	player->drawindics(vw);
	{
		char buf[128];
		GLpmatrix pm;
		projection((glLoadIdentity(), glOrtho(0, gvp.w, gvp.h, 0, -1, 1)));
		glPushMatrix();
		glLoadIdentity();
		war_draw(*vw, player, player->cs, &WarField::drawOverlay);
		glColor4f(1,1,1,1);
		glTranslatef(0,0,-1);
#ifdef _DEBUG
		static int dstrallocs = 0;
//		sprintf(buf, "dstralloc: %d, %d", cpplib::dstring::allocs - dstrallocs, cpplib::dstring::allocs);
		sprintf(buf, "dstralloc: %d, %d", gltestp::dstring::allocs - dstrallocs, gltestp::dstring::allocs);
		diprint(buf, 0, gvp.h - 12.);
//		dstrallocs = cpplib::dstring::allocs;
		dstrallocs = gltestp::dstring::allocs;
#endif
		sprintf(buf, "%s %s", player->cs->classname(), player->cs->name);
		diprint(buf, 0, gvp.h);
		if(player->cs && player->cs->w){
/*			int y = 0;
			int ce;
			int cb;
			sprintf(buf, "E %d", ce = pl.cs->w->countEnts<&WarField::el>());
			diprint(buf, 0, y += 12);
			sprintf(buf, "B %d", cb = pl.cs->w->countEnts<&WarField::bl>());
			diprint(buf, 0, y += 12);
			sprintf(buf, "EB %d", ce * cb);
			diprint(buf, 0, y += 12);
			sprintf(buf, "EE %d", ce * ce);
			diprint(buf, 0, y += 12);
			sprintf(buf, "Bk %d", bullet_kills);
			diprint(buf, 0, y += 12);
			sprintf(buf, "Mk %d", missile_kills);
			diprint(buf, 0, y += 12);
			sprintf(buf, "Wk %d", wire_kills);
			diprint(buf, 0, y += 12);
			sprintf(buf, "hits %d / shoots %d = %g", bullet_hits, bullet_shoots, bullet_shoots ? (double)bullet_hits / bullet_shoots : 0.);
			diprint(buf, 0, y += 12);*/
#ifdef _DEBUG
/*			if(tent3d_fpol_list * tepl = pl.cs->w->getTefpol3d())
				diprint(cpplib::dstring() << "tepl " << Tefpol3DDebug(tepl)->tefpol_c << " " << Tefpol3DDebug(tepl)->tefpol_m << " " << Tefpol3DDebug(tepl)->tevert_c, 0, y += 12);
			if(WarSpace *ws = *pl.cs->w)
				if(tent3d_line_list * tell = ws->gibs)
					diprint(cpplib::dstring() << "tell " << Teline3DDebug(tell)->teline_c << " " << Teline3DDebug(tell)->teline_m << " " << Teline3DDebug(tell)->teline_s << " " << Teline3DDebug(tell)->drawteline, 0, y += 12);
*/
#endif
		}
		sprintf(buf, "Frame rate: %6.2lf fps", vw->dt ? 1. / vw->dt : 0.);
		diprint(buf, gvp.w - 8 * strlen(buf), 12);
		sprintf(buf, "wdtime: %10.8lf/%10.8lf sec", wdtime, vw->dt);
		diprint(buf, gvp.w - 8 * strlen(buf), 24);
		sprintf(buf, "watime: %10.8lf/%10.8lf sec", watime, vw->dt);
		diprint(buf, gvp.w - 8 * strlen(buf), 36);
		if(universe->paused){
			diprint("PAUSED", gvp.w / 2 - 8 * sizeof"PAUSED" / 2, gvp.h / 2 + 20);
		}
		glPopMatrix();
	}
	if(!mouse_captured && !glwfocus && !GLwindow::getCaptor() && s_mousedragx != s_mousex && s_mousedragy != s_mousey){
		int x0 = MIN(s_mousedragx, s_mousex);
		int x1 = MAX(s_mousedragx, s_mousex) + 1;
		int y0 = MIN(s_mousedragy, s_mousey);
		int y1 = MAX(s_mousedragy, s_mousey) + 1;

/*		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(g_left, g_right, g_bottom, g_top, g_near, g_far);
		glMatrixMode(GL_MODELVIEW);*/
		glPushMatrix();
		glLoadIdentity();
		Mat4d rot = player->getrot().tomat4();
		glMultMatrixd(rot);
		gldTranslaten3dv(vw->pos);

		/// Temporary object to draw objects that are being box selected.
		class PreviewSelect : public select_box_callback{
		public:
			virtual void onEntity(Entity *pt){
				double (*cuts)[2];
				int i;
				cuts = CircleCuts(16);
				glPushMatrix();
				gldTranslate3dv(pt->pos);
				glMultMatrixd(irot);
				gldScaled(pt->hitradius());
				glBegin(GL_LINE_LOOP);
				for(i = 0; i < 16; i++)
					glVertex2dv(cuts[i]);
				glEnd();
				glPopMatrix();
			}
			Mat4d irot;
			PreviewSelect(const Mat4d &rot) : irot(rot.transpose()){}
		};

		select_box((2. * x0 / gvp.w - 1.) * gvp.w / gvp.m, (2. * x1 / gvp.w - 1.) * gvp.w / gvp.m,
			-(2. * y1 / gvp.h - 1.) * gvp.h / gvp.m, -(2. * y0 / gvp.h - 1.) * gvp.h / gvp.m, rot, 1, &PreviewSelect(rot));
		glPopMatrix();
/*		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(proj);
		glMatrixMode(GL_MODELVIEW);*/

		glPushMatrix();
		glLoadIdentity();
		glTranslated(0, gvp.h, 0);
		glScaled(1, -1, 1);
		projection((glPushMatrix(), glLoadIdentity(), glOrtho(0, gvp.w, 0, gvp.h, -1, 1)));
		glBegin(GL_LINE_LOOP);
		glVertex2d(s_mousex, s_mousey);
		glVertex2d(s_mousedragx, s_mousey);
		glVertex2d(s_mousedragx, s_mousedragy);
		glVertex2d(s_mousex, s_mousedragy);
		glEnd();
		projection(glPopMatrix());
		glPopMatrix();
	}

	// Guide for tactical rotation
	if(mouse_captured && player->mover == player->tactical){
		Quatd ort = player->cs->w && (WarSpace*)*player->cs->w ? ((WarSpace*)*player->cs->w)->orientation(player->cpos) : quat_u;
		for(int i = 0; i < 12; i++) for(int j = -3; j < 3; j++){
			glPushMatrix();
			glLoadMatrixd(vw->rot * ort.tomat4());
			glRotatef(j * 360 / 12, 0, 1, 0);
			glRotatef(i * 360 / 12, 1, 0, 0);
			glBegin(GL_LINES);
			glVertex3f(  0, -.1, -1);
			glVertex3f(  0,  .1, -1);
			glVertex3f(-.1,   0, -1);
			glVertex3f( .1,   0, -1);
			glEnd();
			glPopMatrix();
		}
	}
}

void WarDrawInt::war_draw_int(){
	if(!this->w || !this->vw)
		return;
	Viewer localvw = gvw;
	Viewer &vw = *this->vw;
//	WarDraw wd;

	/// Convert from the CoorSys the Viewer belongs to the CoordSys the Entity is being drawn.
	localvw.cs = cs;
	localvw.pos = cs->tocs(gvw.pos, gvw.cs);
	localvw.velo = cs->tocsv(gvw.velo, gvw.pos, gvw.cs);
	localvw.qrot = gvw.qrot * cs->tocsq(gvw.cs);
	localvw.relrot = localvw.rot = localvw.qrot.tomat4();
	localvw.relirot = localvw.irot = localvw.qrot.cnj().tomat4();

	GLmatrix ma;
	gldTranslate3dv(vw.cs->tocs(avec3_000, cs));
	gldMultQuat(vw.cs->tocsq(cs).cnj());

	GLcull gc = vw.gc->isOrtho() ? *vw.gc : GLcull(vw.fov, localvw.pos, localvw.irot, vw.gc->getNear(), vw.gc->getFar());
	localvw.gc = &gc;

	/*	wd.lightdraws = 0;
//	wd.maprange = 1.;
	wd.vw = &localvw;
	wd.light = g_light;
	wd.shadowmapping = shadowmapping;
	wd.shader = shaderName;
	wd.shadowmapLoc = shadowmapLoc
	wd.w = *cs->w;*/

	Viewer *oldvw = this->vw;
	this->vw = &localvw;

	(cs->w->*method)(this);

	this->vw = oldvw;
}

/// \param tod Texture of Depth
WarDrawInt::WarDrawInt(Viewer &vw, Player *&player, const CoordSys *cs, void (WarField::* method)(wardraw_t *w), ShadowMap *sm) :
	WarDraw(),
	player(player),
	gvw(vw),
	cs(cs),
	method(method)
{
	this->vw = NULL;
	w = cs->w ? cs->w->operator WarSpace *() : NULL;
	shadowMap = sm;
}

/// Destructor actualy does drawing
void WarDrawInt::draw(){
	if(cs->parent){
		WarDrawInt(gvw, player, cs->parent, method, shadowMap)
			.setViewer(vw)
			.shadowmap(shadowmapping)
			.setShader(shader, textureLoc, shadowmapLoc)
			.draw();
	}
	if(w){
		// Call an internal function to avoid excess use of the stack by recursive calls.
		// It also helps the optimizer to reduce frame pointers.
		war_draw_int();
	}
	if(player->cs == cs && method != &WarField::drawOverlay && !shadowmapping){
		if(shader)
			glUseProgram(0);
		(player->*(method == &WarField::draw ? &Player::draw : &Player::drawtra))(&gvw);
		if(shader)
			glUseProgram(shader);
	}
}

WarDrawInt::~WarDrawInt(){
}

static void cswardraw(const Viewer *vw, CoordSys *cs, void (CoordSys::*method)(const Viewer *)){
	(cs->*method)(vw);
	for(CoordSys *cs2 = cs->children; cs2; cs2 = cs2->next){
		cswardraw(vw, cs2, method);
	}
}



void Game::draw_func(Viewer &vw, double dt){
	int	glerr = glGetError();
	static GLuint fbo = 0, rboId = 0, to = 0;
	static GLuint depthTextures[3] = {0};
	ShadowMap shadowMap;

	glClearDepth(1.);
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	glMultMatrixd(vw.rot);
	GLcull glc[3] = {
		GLcull(vw.fov, vw.pos, vw.irot, g_warspace_near_clip, g_warspace_far_clip),
		GLcull(vw.fov, vw.pos, vw.irot, g_space_near_clip, g_space_far_clip),
		GLcull(vw.fov, vw.pos, vw.irot, g_background_near_clip, g_background_far_clip),
	};
	vw.gclist[0] = &glc[0];
	vw.gclist[1] = &glc[1];
	vw.gclist[2] = &glc[2];
	glPolygonMode(GL_FRONT_AND_BACK, gl_wireframe ? GL_LINE : GL_FILL);
	{
	GLma a(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);
	glColor4f(1,1,1,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	drawstarback(&vw, &galaxysystem, NULL, NULL);
	projection((
		glPushMatrix(), glLoadIdentity(),
		vw.frustum(g_background_near_clip, g_background_far_clip)
	));
	vw.zslice = 2;
	vw.gc = &glc[2];
	universe->startdraw();
	universe->predraw(&vw);
	universe->drawcs(&vw);
	player->draw(&vw);
	projection(glPopMatrix());

	projection((
		glPushMatrix(), glLoadIdentity(),
		vw.frustum(g_space_near_clip, g_space_far_clip)
	));
	vw.zslice = 1;
	vw.gc = &glc[1];
	universe->predraw(&vw);
	universe->drawcs(&vw);
	player->draw(&vw);
	projection(glPopMatrix());
	}

	gldTranslaten3dv(vw.pos);
	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	lightOn();
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);
/*	for(vw.zslice = 1; 0 <= vw.zslice; vw.zslice--)*/{
		projection((
			glPushMatrix(), glLoadIdentity(),
			vw.frustum(g_warspace_near_clip, g_warspace_far_clip)
		));
		Mat4d trans, model, proj;
		glGetDoublev(GL_MODELVIEW_MATRIX, model);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		vw.trans = proj * model;
		vw.zslice = 0;
		vw.gc = &glc[0];

		timemeas_t tm;
		TimeMeasStart(&tm);

/*		GLpmatrix proj;
		double scale = vw.zslice ? 1. : 1e-2;
		projection((
			glLoadIdentity(),
			vw.frustum(scale * g_space_near_clip, scale * g_space_far_clip)
		));
		GLint ie;
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		ie = glGetError();
		glDepthFunc(GL_ALWAYS);
		glClearDepth(1.);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		ie = glGetError();*/
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		if(r_shadows){
			class WarDrawCallback : public ShadowMap::DrawCallback{
			public:
				Game &game;
				const CoordSys *cs;
				ShadowMap *shadowMap;
				WarDrawCallback(Game &game, const CoordSys *cs, ShadowMap *sm) : game(game), cs(cs), shadowMap(sm){}
				void drawShadowMaps(Viewer &vw2){
					cswardraw(&vw2, const_cast<CoordSys*>(game.player->cs), &CoordSys::draw);
					WarDrawInt(vw2, game.player, game.player->cs, &WarField::draw, shadowMap)
						.setViewer(&vw2)
						.shadowmap()
						.draw();
				}
				void draw(Viewer &vw, GLuint shader, GLint textureLoc, GLint shadowmapLoc){
					cswardraw(&vw, const_cast<CoordSys*>(game.player->cs), &CoordSys::draw);
					if(g_shader_enable)
						WarDrawInt(vw, game.player, game.player->cs, &WarField::draw, shadowMap)
							.setViewer(&vw)
							.setShader(shader, textureLoc, shadowmapLoc)
							.draw();
					else
						WarDrawInt(vw, game.player, game.player->cs, &WarField::draw, shadowMap)
							.setViewer(&vw)
							.draw();
				}
			};
			shadowMap.drawShadowMaps(vw, g_light, WarDrawCallback(*this, player->cs, &shadowMap));
		}
		else{
			cswardraw(&vw, const_cast<CoordSys*>(player->cs), &CoordSys::draw);
	//		printf("%lg %d: cswardraw\n", TimeMeasLap(&tm), glGetError());

			WarDraw::init();
			war_draw(vw, player, player->cs, &WarField::draw);
			if(g_shader_enable)
				glUseProgram(0);
		}

#if 0 // buffer copy
		if(!screentex){
			glGenTextures(1, &screentex);
			glBindTexture(GL_TEXTURE_2D, screentex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vw.vp.w, vw.vp.h, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		{
			GLpmatrix pma;
			projection((glLoadIdentity(), vw.frustum(.01, 100.)));
			GLma ma(GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
			glLoadIdentity();
			glLoadMatrixd(vw.rot);
			gldTranslate3dv(-vw.pos);
			glBindTexture(GL_TEXTURE_2D, screentex);
			glReadBuffer(GL_BACK);
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, vw.vp.w, vw.vp.h, 0);
	//		glReadPixels(0, 0, 512, 512, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	//		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	//		glRasterPos2d(0, 0);
	//		glDrawPixels(512, 512, GL_RGB, GL_UNSIGNED_BYTE, pixels);
/*			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glColor4f(.5,.5,.5,1.);
			glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex3d(-1, -1, -2);
			glTexCoord2i(1, 0); glVertex3d( 1, -1, -2);
			glTexCoord2i(1, 1); glVertex3d( 1,  1, -1);
			glTexCoord2i(0, 1); glVertex3d(-1,  1, -1);
			glEnd();*/
		}
		glBindTexture(GL_TEXTURE_2D, 0);
#endif

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		cswardraw(&vw, const_cast<CoordSys*>(player->cs), &CoordSys::drawtra);
		war_draw(vw, player, player->cs, &WarField::drawtra);
		wdtime = TimeMeasLap(&tm);
		projection(glPopMatrix());
	}
	glPopAttrib();
	glPopMatrix();

	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glColor4ub(255, 255, 255, 255);
	drawindics(&vw);
	glLoadIdentity();
	glTranslated(0,0,-1);
	draw_gear(dt);
	glPopAttrib();
	glPopMatrix();

	if(1){
		int minix = 0;
		glPushAttrib(GL_POLYGON_BIT);
		glEnable(GL_BLEND);
		glDisable(GL_LINE_SMOOTH);
		GLwindowState ws;
		ws.w = vw.vp.w;
		ws.h = vw.vp.h;
		ws.m = vw.vp.m;
		ws.mx = s_mousex;
		ws.my = s_mousey;
		glwlist->glwDrawMinimized(ws, player->gametime, &minix);
		glwlist->glwDraw(ws, player->gametime, &minix);
		glPopAttrib();
	}

	if(cmdwnd)
		CmdDraw(vw.vp);

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
	projection((glPushMatrix(), glLoadIdentity()));
	if(mouse_tracking && !mouse_captured){
		static GLuint tex = 0;
		struct MouseCursor{
			int x0, y0;
			int x1, y1;
			int px, py;
		};
		static const MouseCursor mc_normal = {0, 0, 32, 32};
		static const MouseCursor mc_chasecamera = {0, 32, 32, 64};
		static const MouseCursor mc_attack = {32, 0, 64, 32};
		static const MouseCursor mc_forceattack = {32, 32, 64, 64};
		static const MouseCursor mc_horizsize = {64, 32, 96, 64};
		static const MouseCursor mc_vertsize = {96, 32, 128, 64};
//		GLuint oldtex;
		static int texw = 128, texh = 128;

		const gltestp::TexCacheBind *stc;
		stc = gltestp::FindTexture("pointer.bmp");
		if(!stc){
			suftexparam_t stp;
			stp.bmi = NULL;
			stp.flags = STP_ENV | STP_MAGFIL | STP_ALPHA | STP_ALPHA_TEST | STP_TRANSPARENTCOLOR;
			stp.env = GL_MODULATE;
			stp.mipmap = 0x80;
			stp.alphamap = 1;
			stp.magfil = GL_LINEAR;
			stp.transparentColor = 0;
//			tex = CacheSUFMTex("pointer.bmp", &stp, NULL);
			tex = CallCacheBitmap("pointer.bmp", "pointer.bmp", &stp, NULL);
			if(stp.bmi){
				texw = stp.bmi->bmiHeader.biWidth;
				texh = stp.bmi->bmiHeader.biHeight;
			}
		}
		GLattrib attrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

		bool attackorder = MotionGet() & PL_CTRL || player->attackorder || player->forceattackorder;
		static const MouseCursor *mc0[2][2] = {&mc_normal, &mc_attack, &mc_chasecamera, &mc_forceattack};
		static const MouseCursor mcg0[] = {
			mc_normal,
			{64, 0, 96, 32, 16, 16},
			{64, 0, 96, 32, 16, 16},
			{0},
			{96, 0, 128, 32, 16, 16},
			{64, 32, 96, 64, 16, 16},
			{96, 32, 128, 64, 16, 16},
			{0},
			{96, 0, 128, 32, 16, 16},
			{96, 32, 128, 64, 16, 16},
			{64, 32, 96, 64, 16, 16},
			{0},
		};
		int mstate = GLwindow::glwMouseCursorState(s_mousex, s_mousey);
		const MouseCursor &mc = mstate ? mcg0[mstate] : *mc0[MotionGet() & PL_ALT || player->forceattackorder][attackorder];

		glPushMatrix();
		glLoadIdentity();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glTranslated(2. * (s_mousex - mc.px) / vw.vp.w - 1.,
					-2. * (s_mousey - mc.py) / vw.vp.h - - 1., -1);
		glScaled(2. * 32. / vw.vp.w, 2. * 32. / vw.vp.h, 1.);
		glEnable(GL_TEXTURE_2D);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();
		glScaled(1. / texw, 1. / texh, 1.);
		glTranslatef(mc.x0, texh - mc.y1, 0);
		glScaled(mc.x1 - mc.x0, mc.y1 - mc.y0, 1.);
//		glScaled(texw / (mc.x1 - mc.x0), texh / (mc.y1 - mc.y0), 1.);
//		glScaled(.5, .5, 1.);
		glCallList(tex);
		glColor4ub(255,255,255,255);
		glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex2d(-0., -1.);
		glTexCoord2d(1, 0); glVertex2d( 1., -1.);
		glTexCoord2d(1, 1); glVertex2d( 1.,  0.);
		glTexCoord2d(0, 1); glVertex2d(-0.,  0.);
		glEnd();
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	projection(glPopMatrix());
	glPopAttrib();

	glFlush();

	video_frame();
}

static JoyStick joyStick(JOYSTICKID1);
//static JoyStick joyStick2(JOYSTICKID2);



static POINT mouse_pos = {0, 0};

void Game::display_func(void){
	static int init = 0;
	static timemeas_t tm;
	static double gametime = 0.;
	double dt = 0.;
	if(!init){
		extern double dwo;
		init = 1;

		MultiTextureInit();

//		anim_sun(0.);
		universe->anim(0.);

		sqa_anim0();

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
			rdt = (t1 - gametime) * universe->timescale;

		dt = !init ? 0. : rdt < 1. ? rdt : 1.;

		sqa_anim(dt);

		int mousedelta[2] = {0, 0};
		if(mouse_captured){
			POINT p;
			if(GetActiveWindow() != WindowFromDC(wglGetCurrentDC())){
				mouse_captured = 0;
				while(ShowCursor(TRUE) <= 0);
			}
			if(GetCursorPos(&p) && (p.x != mouse_pos.x || p.y != mouse_pos.y)){
				mousedelta[0] = p.x - mouse_pos.x;
				mousedelta[1] = p.y - mouse_pos.y;
				player->rotateLook(p.x - mouse_pos.x, p.y - mouse_pos.y);
				SetCursorPos(mouse_pos.x, mouse_pos.y);
			}
		}

		MotionFrame(dt);

		MotionAnim(*player, dt, flypower);

		player->anim(dt);

#if 0
#define TRYBLOCK(a) {try{a;}catch(std::exception e){fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());}catch(...){fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);}}
#else
#define TRYBLOCK(a) (a);
#endif
		if(!universe->paused) try{
			timemeas_t tm;
			TimeMeasStart(&tm);
			TRYBLOCK(anim(dt));
			TRYBLOCK(GLwindow::glwpostframe());
			TRYBLOCK(universe->endframe());
			TRYBLOCK(client->universe->clientUpdate(dt));
			watime = TimeMeasLap(&tm);
		}
		catch(std::exception e){
			fprintf(stderr, __FILE__"(%d) Exception %s\n", __LINE__, e.what());
		}
		catch(...){
			fprintf(stderr, __FILE__"(%d) Exception ?\n", __LINE__);
		}

		if(glwfocus && cmdwnd)
			glwfocus = NULL;

		input_t inputs;
		inputs.press = MotionGet();
		inputs.change = MotionGetChange();
		if(joyStick.InitJoystick()){
			joyStick.CheckJoystick(inputs);
		}
		(*player->mover)(inputs, dt);
		if(player->nextmover && player->nextmover != player->mover){
/*			Vec3d pos = pl.pos;
			Quatd rot = pl.rot;*/
			(*player->nextmover)(inputs, dt);
/*			pl.pos = pos * (1. - pl.blendmover) + pl.pos * pl.blendmover;
			pl.rot = rot.slerp(rot, pl.rot, pl.blendmover);*/
		}

		if(player->chase && player->controlled == player->chase){
			inputs.analog[1] += mousedelta[0];
			inputs.analog[0] += mousedelta[1];
			player->chase->control(&inputs, dt);
		}

		if(player->controlled)
			glwfocus = NULL;

		// Really should be in draw method, since windows are property of the client.
		glwlist->glwAnim(dt);

		// Quite suspicious about position, but at least postframe opportunity must be kept.
		GLwindow::glwEndFrame();

		gametime = t1;
	}

#ifdef _WIN32
	if(!IsWindowVisible(hWndApp))
		return;
#endif

	{
		extern std::vector<unsigned long> deleteque;
		SerializeMap map;
		map[NULL] = 0;
		map[player] = 1;
		Serializable* visit_list = NULL;
		SerializeContext sc0(*(SerializeStream*)NULL, map, visit_list);
		universe->dive(sc0, &Serializable::map);

		BinSerializeStream bss;
		bss << (unsigned long)deleteque.size();
		std::vector<unsigned long>::iterator it = deleteque.begin();
		for(; it != deleteque.end(); it++)
			bss << *it;
		deleteque.clear();

		SerializeContext sc(bss, map, visit_list);
		bss.sc = &sc;
		player->idPackSerialize(sc);
		universe->dive(sc, &Serializable::idPackSerialize);
		(sc.visit_list)->clearVisitList();


/*		delete client;
		client = new Game();*/
		{
			std::vector<Serializable*> map;
			map.push_back(NULL);
			map.push_back(client->player);
			map.push_back(client->universe);
			BinUnserializeStream bus0((const unsigned char*)bss.getbuf(), bss.getsize());
			UnserializeContext usc(bus0, Serializable::ctormap(), map);
			bus0.usc = &usc;
			client->universe->csIdUnmap(usc);
			{
				BinUnserializeStream bus((const unsigned char*)bss.getbuf(), bss.getsize());
				UnserializeContext usc(bus, Serializable::ctormap(), map);
				bus.usc = &usc;
				client->universe->csIdUnserialize(usc);
			}
		}
	}

	client->clientDraw(gametime, dt);
}
	
void Game::clientDraw(double gametime, double dt){
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
		if(vp[2] <= vp[0] || vp[3] <= vp[1])
			return;
		viewer.vp.set(vp);
		viewer.fov = player->fov;
		double dnear = g_warspace_near_clip, dfar = g_warspace_far_clip;
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
	viewer.cs = player->cs;
	viewer.qrot = player->getrot();
	viewer.rot = viewer.qrot.tomat4();
	viewer.irot = viewer.qrot.cnj().tomat4();
	viewer.pos = player->getpos();
	viewer.relrot = viewer.rot;
	viewer.relirot = viewer.irot;
	viewer.viewtime = gametime;
	viewer.velo = player->getvelo();
	viewer.dt = dt;
	viewer.mousex = s_mousex;
	viewer.mousey = s_mousey;
	viewer.dynamic_range = r_dynamic_range;
	draw_func(viewer, dt);
}

void entity_popup(Entity *pt, GLwindowState &ws, int selectchain){
	PopupMenu *menus = new PopupMenu;
	menus->append(sqa_translate("Halt"), '\0', "halt")
		.append(sqa_translate("Move"), '\0', "moveorder")
		.append(sqa_translate("Chase Camera"), '\0', "chasecamera")
		.append(sqa_translate("Properties"), '\0', "property");
	GLwindow *glw;
	if(selectchain){
		for(; pt; pt = pt->selectnext)
			pt->popupMenu(*menus);
		glw = glwPopupMenu(ws, *menus);
	}
	else if(pt){
		pt->popupMenu(*menus);
		glw = glwPopupMenu(ws, *menus);
	}
	else
		glw = glwPopupMenu(ws, *menus);
	delete menus;
}

/** \brief Box selection routine that can execute any operation via callback. 
 * \param x0,x1,y0,y1 Selection box in screen coordinates.
 * \param rot Camera transformation matrix.
 * \param flags 4 - point selection, which means up to 1 Entity enumeration.
 * \param sbc Callback functionoid that receives enumerated Entities in selection box.
 * \return if any Entity is enumerated */
bool Game::select_box(double x0, double x1, double y0, double y1, const Mat4d &rot, unsigned flags, select_box_callback *sbc){
	Entity *pt;
	Mat4d mat, mat2;
//	int viewstate = 0;
	bool pointselect = !!(flags & 4);
	double best = 1e50;
	double g_far = g_warspace_far_clip, g_near = g_warspace_near_clip;
	Entity *ptbest = NULL;

	if(!player->cs->w)
		return false;

	WarField *w = player->cs->w;

/*	if(g_ally_view){
		entity_t *pt;
		extern struct player *ppl;
		extern entity_t **rstations;
		extern int nrstations;
		int i;
		for(pt = w->tl; pt; pt = pt->next) if(pt->race == ppl->race){
			viewstate = 2;
			break;
		}
		if(viewstate < 1) for(i = 0; i < nrstations; i++) if(rstations[i]->w == w){
			viewstate = 1;
			break;
		}
		if(!viewstate)
			return;
	}*/
/*	if(x1 < x0){
		double tmp = x1;
		x1 = x0;
		x0 = tmp;
	}
	if(y1 < y0){
		double tmp = y1;
		y1 = y0;
		y0 = tmp;
	}*/
	mat[0] = 2 /** g_near*/ / (x1 - x0), mat[4] = 0., mat[8] = (x1 + x0) / (x1 - x0), mat[12] = 0.;
	mat[1] = 0., mat[5] = 2 /** g_near*/ / (y1 - y0), mat[9] = (y1 + y0) / (y1 - y0), mat[13] = 0.;
	mat[2] = 0., mat[6] = 0., mat[10] = -(g_far + g_near) / (g_far - g_near), mat[14] = -2. * g_far * g_near / (g_far - g_near);
	mat[3] = 0., mat[7] = 0., mat[11] = -1., mat[15] = 0.;
	mat2 = mat * rot;
	Vec3d plpos = player->getpos();
	bool ret = false;

	// Cycle through both Entity list and Bullet list, but only ones that tells its selectable.
	static Entity *WarField::*const list[2] = {&WarField::el, &WarField::bl};
	for(int li = 0; li < 2; li++)
	for(pt = player->cs->w->*list[li]; pt; pt = pt->next) if(pt->w && pt->isSelectable()){
		Vec4d lpos, dpos;
		double sp;
		double scradx, scrady;
		dpos = pt->pos - plpos;
		dpos[3] = 1.;
		lpos = mat2.vp(dpos);
		if(lpos[3] == 0.)
			continue;
		lpos[0] /= lpos[3];
		lpos[1] /= lpos[3];
		lpos[2] /= lpos[3];
		sp = -(dpos[0] * rot[2] + dpos[1] * rot[6] + dpos[2] * rot[10])/*VECSP(&rot[8], dpos)*/;
		scradx = pt->hitradius() / sp * 2 / (x1 - x0);
		scrady = pt->hitradius() / sp * 2 / (y1 - y0);
		if(-1 < lpos[0] + scradx && lpos[0] - scradx < 1 && -1 < lpos[1] + scrady && lpos[1] - scrady < 1 && -1 < lpos[2] && lpos[2] < 1 /*((struct entity_private_static*)pt->vft)->hitradius)*/){

			// Leave all detail to callback
			if(!pointselect && sbc)
				sbc->onEntity(pt);

			// If only one Entity is to be selected, find the nearest one to the camera.
			if(pointselect){
				double size = pt->hitradius() + sp;
				if(0 <= size && size < best){
					best = size;
					ptbest = pt;
				}
			}
		}
	}

	// Leave all detail to callback
	if(pointselect && ptbest && sbc)
		sbc->onEntity(ptbest);

	return ret;
}

void capture_mouse(){
	int c;
	extern HWND hWndApp;
	HWND hwd = hWndApp;
	RECT r;
	s_mouseoldx = s_mousex;
	s_mouseoldy = s_mousey;
	GetClientRect(hwd, &r);
	mouse_pos.x = (r.left + r.right) / 2;
	mouse_pos.y = (r.top + r.bottom) / 2;
	ClientToScreen(hwd, &mouse_pos);
	SetCursorPos(mouse_pos.x, mouse_pos.y);
	c = ShowCursor(FALSE);
	while(0 <= c)
		c = ShowCursor(FALSE);
	glwfocus = NULL;
	mouse_captured = true;
}

static void uncapture_mouse(){
	extern HWND hWndApp;
	HWND hwd = hWndApp;
	mouse_captured = 0;
	s_mousedragx = s_mouseoldx;
	s_mousedragy = s_mouseoldy;
	mouse_pos.x = s_mouseoldx;
	mouse_pos.y = s_mouseoldy;
	ClientToScreen(hwd, &mouse_pos);
	SetCursorPos(mouse_pos.x, mouse_pos.y);
/*	while(ShowCursor(TRUE) < 0);*/
}

int cmd_halt(int, char *[], void *pv){
	Player *pl = (Player*)pv;
	for(Entity *pt = pl->selected; pt; pt = pt->selectnext)
		pt->command(&HaltCommand());
	return 0;
}

int cmd_move(int argc, char *argv[], void *pv){
	Player *pl = (Player*)pv;
	Entity *pt;
	if(argc < 4 || !pl->cs)
		return 0;
	MoveCommand com;
	Vec3d comdst;
	comdst[0] = atof(argv[1]);
	comdst[1] = atof(argv[2]);
	comdst[2] = atof(argv[3]);
	com.destpos = comdst;
	Quatd headrot;
	int n = 0;
	for(pt = pl->selected; pt; pt = pt->selectnext) if(pt->w == pl->cs->w){
		if(n == 0 && DBL_EPSILON < (comdst - pt->pos).slen())
			headrot = Quatd::direction(-(comdst - pt->pos));
		n++;
		Vec3d dp((n % 2 * 2 - 1) * (n / 2 * .05), 0., n / 2 * .05);
		com.destpos = comdst + headrot.trans(dp);
		pt->command(&com);
	}
	return 0;
}



void Game::mouse_func(int button, int state, int x, int y){

	if(cmdwnd){
		CmdMouseInput(button, state, x, y);
		return;
	}

	if(!mouse_captured){
		if(glwdrag){
			if(state == GLUT_UP)
				glwdrag = NULL;
			return;
		}
		GLint vp[4];
		viewport gvp;
		GLwindowState ws;
		glGetIntegerv(GL_VIEWPORT, vp);
		gvp.set(vp);
		ws.set(vp);
		int killfocus = 1, ret = 0;
		ret = GLwindow::mouseFunc(button, state, x, y, ws);

		// If mouse message is processed in the foreground window, clear dragging box on the desktop.
		if(ret){
			s_mousedragx = s_mousex;
			s_mousedragy = s_mousey;
			return;
		}
		if(player->moveorder && button == GLUT_LEFT_BUTTON && state == GLUT_UP){
			char buf[3][64];
			char *args[4] = {"move", buf[0], buf[1], buf[2]};
			Vec3d lpos(player->move_hitpos);
			lpos[2] -= player->move_z;
			Vec3d pos = player->move_rot.vp3(lpos);
/*			VECSUBIN(pos, pl.pos);*/
			if(player->selected)
				pos += player->selected->pos;
			sprintf(buf[0], "%15lg", pos[0]);
			sprintf(buf[1], "%15lg", pos[1]);
			sprintf(buf[2], "%15lg", pos[2]);
			cmd_move(4, args, player);
			player->moveorder = 0;
			return;
		}
		if(!glwfocus && button == GLUT_LEFT_BUTTON && state == GLUT_UP){
			if(/*boxable &&*/ !glwfocus){

				/// Temporary object to select objects.
				class SelectSelect : public select_box_callback{
				public:
					Game &game;
					virtual void onEntity(Entity *e){
						e->selectnext = game.player->selected;
						game.player->selected = e;
					}
					SelectSelect(Game &game) : game(game){ game.player->selected = NULL; }
				};

				/// Temporary object that accumulates and issues attack command to entities.
				class AttackSelect : public select_box_callback{
				public:
					virtual void onEntity(Entity *e){
						com.ents.insert(e);
					}
					Game &game;
					AttackCommand ac;
					ForceAttackCommand fac;
					AttackCommand &com;
					Entity *list;
					AttackSelect(Game &game, bool force, Entity *alist) : game(game), com(force ? fac : ac), list(alist){}
					/// The destructor issues command.
					~AttackSelect(){
						if(!com.ents.empty())
							game.player->attackorder = game.player->forceattackorder = 0;
						for(; list; list = list->selectnext)
							list->command(&com);
					}
				};

				/// Temporary object to select entities to chase the camera on.
				class FocusSelect : public select_box_callback{
				public:
					Game &game;
					virtual void onEntity(Entity *e){
						game.player->chase = e;
						game.player->chases.insert(e);
					}
					FocusSelect(Game &game) : game(game){game.player->chases.clear();}
				};

				int x0 = MIN(s_mousedragx, s_mousex);
				int x1 = MAX(s_mousedragx, s_mousex) + 1;
				int y0 = MIN(s_mousedragy, s_mousey);
				int y1 = MAX(s_mousedragy, s_mousey) + 1;
				Mat4d rot = player->getrot().tomat4();
				bool attacking = MotionGet() & PL_CTRL || player->attackorder || player->forceattackorder;
				bool forced = MotionGet() & PL_ALT && (MotionGet() & PL_CTRL) || player->forceattackorder;
				select_box((2. * x0 / gvp.w - 1.) * gvp.w / gvp.m, (2. * x1 / gvp.w - 1.) * gvp.w / gvp.m,
					-(2. * y1 / gvp.h - 1.) * gvp.h / gvp.m, -(2. * y0 / gvp.h - 1.) * gvp.h / gvp.m, rot,
					((s_mousedragx == s_mousex && s_mousedragy == s_mousey) << 2),
					attacking ? (select_box_callback*)&AttackSelect(*this, forced, player->selected) :
					MotionGet() & PL_ALT ? (select_box_callback*)&FocusSelect(*this) :
					(select_box_callback*)&SelectSelect(*this));
				s_mousedragx = s_mousex;
				s_mousedragy = s_mousey;
			}
		}
		else if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP){
			entity_popup(player->selected, ws, 1);
		}
		glwfocus = NULL;
	}

	if(state == GLUT_DOWN || state == GLUT_UP)
		(state == GLUT_DOWN ? BindExec : BindKeyUp)(button + KEYBIND_MOUSE_BASE + GLUT_LEFT_BUTTON);

	s_mousex = x;
	s_mousey = y;

/*	{
		int ivp[4];
		glGetIntegerv(GL_VIEWPORT, ivp);
		viewport gvp;
		gvp.set(ivp);
		int i = (y - 40) / 10;
		if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && gvp.w - 160 <= x && 0 <= i && i < OV_COUNT){
			if(g_counts[i] == 1){
				pl.chase = pl.selected = g_tanks[i];
			}
			else
				pl.selected = NULL;
			current_vft = g_vfts[i];
		}
	}*/

#if USEWIN && defined _WIN32
	if(!cmdwnd && !player->controlled && (!mouse_captured ? state == GLUT_KEEP_DOWN : state == GLUT_UP) && button == GLUT_RIGHT_BUTTON){
		mouse_captured = !mouse_captured;
/*		printf("right up %d\n", mouse_captured);*/
		if(mouse_captured){
			capture_mouse();
		}
		else{
			uncapture_mouse();
		}
	}
#endif
}

void reshape_func(int w, int h)
{
	// Call before viewport changes.
	GLwindow::reshapeFunc(w, h);

	glViewport(0, 0, w, h);
}

static int cmd_eject(int argc, char *argv[]){
	if(server->player->chase){
		server->player->chase = NULL;
		server->player->freelook->pos += server->player->getrot().cnj() * Vec3d(0,0,.3);
		server->player->mover = server->player->freelook;
	}
	return 0;
}

static int cmd_chasecamera(int argc, char *argv[]){
	if(server->player->selected){
		server->player->chase = server->player->selected;
		server->player->cs = server->player->selected->w->cs;
		server->player->chases.clear();
		for(Entity *e = server->player->selected; e; e = e->selectnext)
			server->player->chases.insert(e);
	}
	return 0;
}

static int cmd_originrotation(int, char *[]){
	server->player->setrot(quat_u);
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
		1,                     // no stencil buffer 
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
	if(glwfocus){
		int ret;
		GLwindow **referrer;
		ret = glwfocus->specialKey(key);
		if(ret)
			return;
	}
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

	if(glwfocus){
		if(key == ESC)
			glwfocus = NULL;
		else
			glwfocus->key(key);
		return;
	}

	// The Esc key is the last resort to exit controlling
	if(key == ESC)
		server->player->uncontrol();

	switch(key){
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			if((g_gear_toggle_mode ? MotionGetToggle : MotionGet)() & PL_G){
				server->player->freelook->setGear(key - '1');
				MotionSetToggle(PL_G, 0);

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
	case VK_SHIFT: BindExec(1);/*key_func(1, 0, 0);*/ break;
	case VK_CONTROL: BindExec(2);/*key_func(2, 0, 0);*/ break;
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
			glsl_register();
/*			if(!SetTimer(hWnd, 2, 10, NULL))
				assert(0);*/
			break;

		case WM_TIMER:
			if(hgl){
				HDC hdc;
//				HGLRC hgl;
				hdc = GetDC(hWnd);
//				hgl = winglstart(hdc);
				server->display_func();
//				wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
				SwapBuffers(hdc);
//				winglend(hgl);
				ReleaseDC(hWnd, hdc);
			}
			break;

		case WM_SIZE:
			if(hgl){
				reshape_func(LOWORD(lParam), HIWORD(lParam));
			}
			break;

		case WM_MOUSELEAVE:
			while(ShowCursor(TRUE) <= 0);
			mouse_tracking = 0;
			break;

		case WM_MOUSEMOVE:
			if(!mouse_tracking){
				TRACKMOUSEEVENT evt;
				evt.cbSize = sizeof evt;
				evt.dwFlags = TME_LEAVE;
				evt.hwndTrack = hWnd;
				TrackMouseEvent(&evt);
				while(0 <= ShowCursor(FALSE));
				mouse_tracking = 1;
			}
			if(!mouse_captured){
				s_mousex = LOWORD(lParam);
				s_mousey = HIWORD(lParam);
				server->player->mousemove(hWnd, s_mousex - s_mousedragx, s_mousey - s_mousedragy, wParam, lParam);
				if(glwdrag || !(wParam & MK_LBUTTON)){
					s_mousedragx = s_mousex;
					s_mousedragy = s_mousey;
				}

				// Send mouse move messages to GLwindow system.
				{
					GLwindowState ws;
					GLint vp[4];
					glGetIntegerv(GL_VIEWPORT, vp);
					ws.set(vp);
					ws.mx = s_mousex;
					ws.my = s_mousey;

					GLwindow::mouseFunc(GLUT_LEFT_BUTTON, wParam & MK_LBUTTON ? GLUT_KEEP_DOWN : GLUT_KEEP_UP, s_mousex, s_mousey, ws);
					GLwindow::mouseFunc(GLUT_RIGHT_BUTTON, wParam & MK_RBUTTON ? GLUT_KEEP_DOWN : GLUT_KEEP_UP, s_mousex, s_mousey, ws);

				}

				// If mouse has been dragged over an window, do not show selection rectangle behind it, no matter the window processes mouse messages or not.
				if(GLwindow::getDragStart()){
					s_mousedragx = s_mousex;
					s_mousedragy = s_mousey;
				}

				if(!glwfocus && (wParam & MK_RBUTTON) && !mouse_captured){
					capture_mouse();
				}
			}
			if(glwdrag){
//				glwindow *glw = glwlist;
				glwdrag->mouseDrag(LOWORD(lParam) - glwdragpos[0], HIWORD(lParam) - glwdragpos[1]);
			}
			break;

		case WM_RBUTTONDOWN:
			server->mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONDOWN:
			server->mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_RBUTTONUP:
			server->mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONUP:
			server->mouse_func(GLUT_LEFT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_MOUSEWHEEL:
			{
				int ret = 0;
				POINT p = {LOWORD(lParam), HIWORD(lParam)};
				ScreenToClient(hWnd, &p);
				server->mouse_func((short)HIWORD(wParam) < 0 ? GLUT_WHEEL_DOWN : GLUT_WHEEL_UP, GLUT_DOWN, p.x, p.y);
			}
			break;

		case WM_CHAR:
			key_func(wParam, 0, 0);
			break;

		/* most inputs from keyboard is processed through WM_CHAR, but some special keys are
		  not sent as characters. */
		case WM_KEYDOWN:
			if(wParam == VK_RETURN)
				BindExec('\n');
			else if(VK_NUMPAD0 <= wParam && wParam <= VK_NUMPAD9)
				BindExec(wParam - VK_NUMPAD0 + 0x90);
			else{
				if(wParam != VK_CONTROL && GetKeyState(VK_CONTROL) & 0x7000 && isprint(wParam))
					BindExec(wParam);
				else
					non_printable_key(hWnd, message, wParam, lParam, 0);
			}
			break;

		case WM_SYSKEYDOWN:
			non_printable_key(hWnd, message, wParam, lParam, 0);
			break;

		/* technique to enable momentary key commands */
		case WM_KEYUP:
			if(wParam == VK_RETURN)
				BindKeyUp('\n');
			else if(VK_NUMPAD0 <= wParam && wParam <= VK_NUMPAD9)
				BindKeyUp(wParam - VK_NUMPAD0 + 0x90);
			else{
				if(wParam != VK_CONTROL && GetKeyState(VK_CONTROL) & 0x7000)
					BindKeyUp(wParam);
				else
					BindKeyUp(toupper(wParam));
			}
			switch(wParam){
			case VK_DELETE: BindKeyUp(DELETEKEY); break;
//			case VK_ESCAPE: BindKeyUp(ESC); break;
			case VK_ESCAPE: if(mouse_captured){
//				uncapture_mouse();
				mouse_captured = false;
				while(ShowCursor(TRUE) < 0);
				while(0 <= ShowCursor(FALSE));
			}
			case VK_SHIFT: BindKeyUp(1); break;
			case VK_CONTROL: BindKeyUp(2); break;
			case VK_PRIOR: BindKeyUp(GLUT_KEY_PAGE_UP); break;
			case VK_NEXT: BindKeyUp(GLUT_KEY_PAGE_DOWN); break;
			}
			break;

		case WM_SYSKEYUP:
			BindKeyUp(wParam == VK_MENU ? 3 : toupper(wParam));
			switch(wParam){
			case VK_DELETE: BindKeyUp(DELETEKEY); break;
			case VK_ESCAPE: BindKeyUp(ESC); break;
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

static int cmd_exit(int argc, char *argv[]){
#ifdef _WIN32
	// Safely close the window
	PostMessage(hWndApp, WM_SYSCOMMAND, SC_CLOSE, 0);
#else
	exit(0);
#endif
	return 0;
}


extern double g_nlips_factor;


static int cmd_sq(int argc, char *argv[]){
	if(argc < 2){
		CmdPrint("usage: sq \"Squirrel One Linear Source\"");
		return 0;
	}
	HSQUIRRELVM &v = g_sqvm;
	if(SQ_FAILED(sq_compilebuffer(g_sqvm, argv[1], strlen(argv[1]), _SC("sq"), SQTrue))){
		CmdPrint("Compile error");
	}
	else{
		sq_pushroottable(g_sqvm);
		sq_call(g_sqvm, 1, SQFalse, SQTrue);
		sq_pop(v, 1);
	}
	return 0;
}






static Client gcl;


static void SendChat(Client *pc, const char *buf){
	if(pc->mode == Client::ServerWaitGame || pc->mode == Client::ServerGame){
		SendChatServer(&pc->server, buf);
	}
	else if(pc->mode == Client::ClientWaitGame || pc->mode == Client::ClientGame){
		dstring ds = dstring() << "SAY " << buf << "\r\n";
		send(pc->con, ds, ds.len(), 0);
	}
}

static int cmd_say(int argc, char *argv[]){
	dstring text;
	for(int i = 1; i < argc; i++){
		if(i != 1)
			text << " ";
		text << argv[i];
	}
	SendChat(&gcl, text);
	return 0;
}



int main(int argc, char *argv[])
{
#if defined _WIN32 && defined _DEBUG
	// Unmask invalid floating points to detect bugs
	unsigned flags = _controlfp(0,_EM_INVALID|_EM_ZERODIVIDE);
	printf("controlfp %p\n", flags);
#endif
	HSQUIRRELVM &v = g_sqvm;

//	glwcmdmenu = glwMenu("Command Menu", 0, NULL, NULL, NULL, 1);
//	glwfocus = NULL;
//	pl.cs = &universe;
	server = new ServerGame();
	client = new Game();

	viewport vp;
	CmdInit(&vp);
	MotionInit();
	CmdAdd("bind", cmd_bind);
	CmdAdd("pushbind", cmd_pushbind);
	CmdAdd("popbind", cmd_popbind);
	CmdAdd("toggleconsole", cmd_toggleconsole);
	CmdAdd("eject", cmd_eject);
	CmdAdd("exit", cmd_exit);
	CmdAdd("originrotation", cmd_originrotation);
//	CmdAddParam("addcmdmenuitem", GLWmenu::cmd_addcmdmenuitem, (void*)glwcmdmenu);
	extern int cmd_togglesolarmap(int argc, char *argv[], void *);
	CmdAddParam("togglesolarmap", cmd_togglesolarmap, &server->player);
	extern int cmd_togglewarpmenu(int argc, char *argv[], void *);
	CmdAddParam("togglewarpmenu", cmd_togglewarpmenu, &server->player);
	extern int cmd_transit(int argc, char *argv[], void *pv);
	CmdAddParam("transit", cmd_transit, &server->player);
	extern int cmd_warp(int argc, char *argv[], void *pv);
	CmdAddParam("warp", cmd_warp, &server->player);
	CmdAdd("chasecamera", cmd_chasecamera);
	CmdAddParam("property", Entity::cmd_property, &server->player);
	extern int cmd_armswindow(int argc, char *argv[], void *pv);
	CmdAddParam("armswindow", cmd_armswindow, &server->player);
	CmdAddParam("save", Universe::cmd_save, server->universe);
	CmdAddParam("load", Universe::cmd_load, server->universe);
//	CmdAddParam("buildmenu", cmd_build, &pl);
	CmdAddParam("dockmenu", cmd_dockmenu, &server->player);
	CmdAdd("sq", cmd_sq);
	class Refresh{
	public:
		static int refresh(int argc, char *argv[]){
			delete openGLState;
			openGLState = new OpenGLState;
			return 1;
		}
	};
	CmdAdd("refresh", &Refresh::refresh);
	CmdAdd("video", video);
	CmdAdd("video_stop", video_stop);
	CmdAdd("say", cmd_say);
	CoordSys::registerCommands(server->player);
	CvarAdd("gl_wireframe", &gl_wireframe, cvar_int);
	CvarAdd("g_gear_toggle_mode", &g_gear_toggle_mode, cvar_int);
	CvarAdd("g_drawastrofig", &show_planets_name, cvar_int);
	CvarAdd("pause", &server->universe->paused, cvar_int);
	CvarAdd("g_timescale", &server->universe->timescale, cvar_double);
	CvarAdd("viewdist", &server->player->viewdist, cvar_double);
	CvarAdd("g_otdrawflags", &WarSpace::g_otdrawflags, cvar_int);
	CvarAdd("seat", &server->player->chasecamera, cvar_int);
	CvarAdd("pid_pfactor", &Sceptor::pid_pfactor, cvar_double);
	CvarAdd("pid_ifactor", &Sceptor::pid_ifactor, cvar_double);
	CvarAdd("pid_dfactor", &Sceptor::pid_dfactor, cvar_double);
	CvarAdd("g_nlips_factor", &g_nlips_factor, cvar_double);
	CvarAdd("g_astro_timescale", &OrbitCS::astro_timescale, cvar_double);
	CvarAdd("g_space_near_clip", &g_space_near_clip, cvar_double);
	CvarAdd("g_space_far_clip", &g_space_far_clip, cvar_double);
	CvarAdd("g_warspace_near_clip", &g_warspace_near_clip, cvar_double);
	CvarAdd("g_warspace_far_clip", &g_warspace_far_clip, cvar_double);
	CvarAddVRC("g_shader_enable", &g_shader_enable, cvar_int, (int(*)(void*))vrc_shader_enable);
	CvarAdd("r_exposure", &r_dynamic_range, cvar_double);
	CvarAdd("r_orbit_axis", &r_orbit_axis, cvar_int);
	CvarAdd("r_shadows", &r_shadows, cvar_int);
	CvarAdd("g_fix_dt", &g_fix_dt, cvar_double);
	Player::cmdInit(*server->player);

	sqa_init(server);

	{
		const SQChar *s = _SC("space.dat");
		StackReserver sr(v);
		sq_pushroottable(v); // root
		sq_pushstring(v, _SC("stellar_file"), -1); // root "init_Universe"
		if(SQ_SUCCEEDED(sq_get(v, -2))){ // root closure
			sq_getstring(v, -1, &s);
		}
		StellarFileLoad(s, server->universe);
	}
	CmdExec("@exec autoexec.cfg");

	if(1 < argc && !strcmp(argv[1], "/client"))
		gcl.joingame();
	else
		gcl.hostgame();

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

		do{
			if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
				if(GetMessage(&msg, NULL, 0, 0) <= 0)
					break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				SendMessage(hWnd, WM_TIMER, 0, 0);
		}while (true);
	}
#else
/*	printf("%lg %lg %lg\n", fmod(-8., 5.), -8. / 5. - (int)(-8. / 5.), -8. / 5. - (floor)(-8. / 5.));*/
/*	display_func();*/
	glutMainLoop();
#endif

	while(ShowCursor(TRUE) < 0);
	while(0 <= ShowCursor(FALSE));

	// We need to free the universe here, or class static objects can be freed before all instances are deleted.
	// The chance depends on the compiler condition which is not stable.
	delete server;

	sqa_exit();

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