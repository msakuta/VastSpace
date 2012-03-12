#include "Warpable.h"
#include "Player.h"
#include "Bullet.h"
#include "CoordSys.h"
#include "Viewer.h"
#include "cmd.h"
#ifdef _WIN32
#include "glw/glwindow.h"
#include "glw/GLWmenu.h"
#endif
#include "judge.h"
#include "astrodef.h"
#include "stellar_file.h"
#include "astro_star.h"
#include "serial_util.h"
#include "glstack.h"
#include "EntityCommand.h"
#include "btadapt.h"
#include "arms.h"
#include "Docker.h"
//#include "sensor.h"
#include "motion.h"
#include "draw/WarDraw.h"
#include "glsl.h"
#include "dstring.h"
extern "C"{
#ifdef _WIN32
#include "bitmap.h"
#endif
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/mathdef.h>
#include <clib/suf/sufbin.h>
#ifdef _WIN32
#include <clib/suf/sufdraw.h>
#include <clib/GL/gldraw.h>
#include <clib/GL/multitex.h>
#include <clib/wavsound.h>
#endif
#include <clib/zip/UnZip.h>
}
#include <cpplib/vec4.h>
#include <assert.h>
#include <string.h>
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"

#include "LinearMath/btQuaternion.h"
#include "LinearMath/btTransform.h"






/* some common constants that can be used in static data definition. */
#define SQRT_2 1.4142135623730950488016887242097
#define SQRT_3 1.4422495703074083823216383107801
#define SIN30 0.5
#define COS30 0.86602540378443864676372317075294
#define SIN15 0.25881904510252076234889883762405
#define COS15 0.9659258262890682867497431997289



int g_healthbar = 1;
double g_capacitor_gen_factor = 1.;

/* color sequences */
#if 0
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{2.3, COLOR32RGBA(255,31,0,63)},
};
static const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 3.);
static const struct color_node cnl_shortburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.25, COLOR32RGBA(255,127,31,0)},
};
static const struct color_sequence cs_shortburn = DEFINE_COLSEQ(cnl_shortburn, (COLOR32)-1, 0.5);
#endif




#ifdef NDEBUG
#else
void hitbox_draw(const Entity *pt, const double sc[3], int hitflags){
	glPushMatrix();
	glScaled(sc[0], sc[1], sc[2]);
	glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_POLYGON_SMOOTH);
	glColor4ub(255,0,0,255);
	glBegin(GL_LINES);
	{
		int i, j, k;
		for(i = 0; i < 3; i++) for(j = -1; j <= 1; j += 2) for(k = -1; k <= 1; k += 2){
			double v[3];
			v[i] = j;
			v[(i+1)%3] = k;
			v[(i+2)%3] = -1.;
			glVertex3dv(v);
			v[(i+2)%3] = 1.;
			glVertex3dv(v);
		}
	}
/*	for(int ix = 0; ix < 2; ix++) for(int iy = 0; iy < 2; iy++) for(int iz = 0; iz < 2; iz++){
		glColor4fv(hitflags & (1 << (ix * 4 + iy * 2 + iz)) ? Vec4<float>(1,0,0,1) : Vec4<float>(0,1,1,1));
		glVertex3dv(vec3_000);
		glVertex3dv(1.2 * Vec3d(ix * 2 - 1, iy * 2 - 1, iz * 2 - 1));
	}*/
	glEnd();
	glPopAttrib();
	glPopMatrix();
}
#endif


void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g){
	double x = v * 2. - 1., h = MIN(.1, .1 / (1. + scale)), hs = h / 2.;
	if(!g_healthbar || !Player::g_overlay || wd->shadowmapping)
		return;
	if(g_healthbar == 1 && wd->w && wd->w->pl){
		if(wd->w->pl->selected.find(pt) == wd->w->pl->selected.end())
			return;
	}
	if(wd->shader)
		glUseProgram(0);
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	if(glActiveTextureARB){
		for(int i = 1; i < 5; i++){
			glActiveTextureARB(GL_TEXTURE0_ARB + i);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
	glPushMatrix();
	gldTranslate3dv(pt->pos);
	glMultMatrixd(wd->vw->irot);
	gldScaled(scale);
#if 0 /* signal spectrum drawing */
	if(vft->signalSpectrum){
		double x;
		glColor4ub(0,255,127,255);
		glBegin(GL_LINES);
		for(x = -10; x < 10; x++){
			glVertex2d(x / 10., -1);
			glVertex2d(x / 10., -.95);
		}
		glEnd();
		glBegin(GL_LINE_STRIP);
		for(x = -100; x < 100; x++){
			glVertex2d(x / 100., -1 + vft->signalSpectrum(pt, x / 10.));
		}
		glEnd();
	}
#endif
	glBegin(GL_QUADS);
	glColor4ub(0,255,0,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( x, 1., 0.);
	glVertex3d( x, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glColor4ub(255,0,0,255);
	glVertex3d( x, 1., 0.);
	glVertex3d( 1., 1., 0.);
	glVertex3d( 1., 1. - h, 0.);
	glVertex3d( x, 1. - h, 0.);
	if(0 <= s){
		x = s * 2. - 1.;
		glColor4ub(0,255,255,255);
		glVertex3d(-1., 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( x, 1. + hs, 0.);
		glVertex3d(-1., 1. + hs, 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs * 2, 0.);
		glVertex3d( 1., 1. + hs, 0.);
		glVertex3d( x, 1. + hs, 0.);
	}
	glEnd();
	if(0 <= g){
		x = g * 2. - 1.;
		glTranslated(0, -2. * h, 0.);
		glBegin(GL_QUADS);
		glColor4ub(255,255,0,255);
		glVertex3d(-1., 1. - hs, 0.);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( x, 1., 0.);
		glVertex3d(-1., 1., 0.);
		glColor4ub(255,0,127,255);
		glVertex3d( x, 1. - hs, 0.);
		glVertex3d( 1., 1. - hs, 0.);
		glVertex3d( 1., 1., 0.);
		glVertex3d( x, 1., 0.);
		glEnd();
	}
/*	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
/*	glBegin(GL_LINE_LOOP);
	glColor4ub(0,255,255,255);
	glVertex3d(-1., 1., 0.);
	glVertex3d( 1, 1., 0.);
	glVertex3d( 1, 1. - h, 0.);
	glVertex3d(-1., 1. - h, 0.);
	glEnd();*/
	glPopMatrix();
	glPopAttrib();
	if(wd->shader)
		glUseProgram(wd->shader);
}


void Warpable::drawtra(wardraw_t *wd){
	if(Player::g_overlay && task == sship_moveto){
		glBegin(GL_LINES);
		glColor4ub(0,0,255,255);
		glVertex3dv(pos);
		glColor4ub(0,255,255,255);
		glVertex3dv(dest);
		glEnd();
	}
}

int cmd_togglewarpmenu(int argc, char *argv[], void *pv){
	Player *player = (Player*)pv;
	extern coordsys *g_galaxysystem;
	char *cmds[64]; /* not much of menu items as 64 are able to displayed after all */
	const char *subtitles[64];
	coordsys *reta[64], **retp = reta;
	static const char *windowtitle = "Warp Destination";
	GLwindow *wnd, **ppwnd;
	int left, i;
	ppwnd = GLwindow::findpp(&glwlist, &GLwindow::TitleCmp(windowtitle));
	if(ppwnd){
		glwActivate(ppwnd);
		return 0;
	}
/*	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}*/
/*	for(Player::teleport_iterator it = Player::beginTeleport(), left = 0; it != Player::endTeleport() && left < numof(cmds); it++){
		teleport *tp = Player::getTeleport(it);
		if(!(tp->flags & TELEPORT_WARP))
			continue;
		cmds[left] = (char*)malloc(sizeof "warp \"\"" + strlen(tp->name));
		strcpy(cmds[left], "warp \"");
		strcat(cmds[left], tp->name);
		strcat(cmds[left], "\"");
		subtitles[left] = tp->name;
		left++;
	}*/
	if(!player)
		return -1;
	PopupMenu pm;
	for(Player::teleport_iterator it = player->beginTeleport(); it != player->endTeleport(); it++){
		teleport *tp = player->getTeleport(it);
		if(!(tp->flags & TELEPORT_WARP))
			continue;
		pm.append(tp->name, 0, gltestp::dstring("warp \"") << tp->name << '"');
	}
//	wnd = glwMenu(windowtitle, left, subtitles, NULL, cmds, 0);
	wnd = glwMenu(windowtitle, pm, GLW_CLOSE | GLW_COLLAPSABLE);
	glwAppend(wnd);
/*	for(i = 0; i < left; i++){
		free(cmds[i]);
	}*/
/*	left = enum_cs_flags(g_galaxysystem, CS_WARPABLE, CS_WARPABLE, &retp, numof(reta));
	{
		int i;
		for(i = 0; i < numof(cmds) - left; i++){
			cmds[i] = malloc(sizeof "warp \"\"" + strlen(reta[i]->name));
			strcpy(cmds[i], "warp \"");
			strcat(cmds[i], reta[i]->name);
			strcat(cmds[i], "\"");
			subtitles[i] = reta[i]->fullname ? reta[i]->fullname : reta[i]->name;
		}
	}
	wnd = glwMenu(numof(reta) - left, subtitles, NULL, cmds, 0);*/
//	wnd->title = windowtitle;
	return 0;
}

int Warpable::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.appendSeparator().append("Warp to...", 'w', "togglewarpmenu");
	return ret;
}

void Warpable::drawHUD(wardraw_t *wd){
	Warpable *p = this;
/*	base_drawHUD_target(pt, wf, wd, gdraw);*/
	st::drawHUD(wd);
	glPushMatrix();
	glPushAttrib(GL_CURRENT_BIT);
	if(warping){
		Vec3d warpdstpos = w->cs->tocs(warpdst, warpdstcs);
		Vec3d eyedelta = warpdstpos - wd->vw->pos;
		eyedelta.normin();
		glLoadMatrixd(wd->vw->rot);
		glRasterPos3dv(eyedelta);
		gldprintf("warpdst");
	}
	glLoadIdentity();

	{
		GLint vp[4];
		double left, bottom, velo;
		GLmatrix glm;

		{
			int w, h, m, mi;
			glGetIntegerv(GL_VIEWPORT, vp);
			w = vp[2], h = vp[3];
			m = w < h ? h : w;
			mi = w < h ? w : h;
			left = -(double)w / m;
			bottom = -(double)h / m;

			velo = warping && warpcs ? warpcs->velo.len() : this->velo.len();
	//		w->orientation(wf, &ort, pt->pos);
			glRasterPos3d(left + 200. / m, -bottom - 100. / m, -1);
			gldprintf("%lg km/s", velo);
			glRasterPos3d(left + 200. / m, -bottom - 120. / m, -1);
			gldprintf("%lg kt", 1944. * velo);
			glScaled((double)mi / m, (double)mi / m, 1);
		}

		if(p->warping){
			double (*cuts)[2];
			char buf[128];
			Vec3d *pvelo = warpcs ? &warpcs->velo : &this->velo;
			Vec3d dstcspos = warpdstcs->tocs(this->pos, w->cs);/* current position measured in destination coordinate system */
			Vec3d warpdst = w->cs->tocs(p->warpdst, p->warpdstcs);

			double velo = (*pvelo).len();
			Vec3d delta = p->warpdst - dstcspos;
			double dist = delta.len();
			p->totalWarpDist = p->currentWarpDist + dist;
			cuts = CircleCuts(32);
			glTranslated(0, -.65, -1);
			glScaled(.25, .25, 1.);

			double f = log10(velo / p->warpSpeed);
			int iphase = -8 < f ? 8 + f : 0;

			/* speed meter */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUAD_STRIP);
			for(int i = 4; i <= 4 + iphase; i++){
				glVertex2d(-cuts[i][1], cuts[i][0]);
				glVertex2d(-.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			for(int i = 4; i <= 12; i++){
				glVertex2d(cuts[i][1], cuts[i][0]);
			}
			for(int i = 12; 4 <= i; i--){
				glVertex2d(.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			glBegin(GL_LINES);
			for(int i = 4; i <= 12; i++){
				glVertex2d(cuts[i][1], cuts[i][0]);
				glVertex2d(.5 * cuts[i][1], .5 * cuts[i][0]);
			}
			glEnd();

			/* progress */
			glColor4ub(127,127,127,255);
			glBegin(GL_QUADS);
			glVertex2d(-1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.5);
			glVertex2d(p->currentWarpDist / p->totalWarpDist * 2. - 1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			glColor4ub(255,255,255,255);
			glBegin(GL_LINE_LOOP);
			glVertex2d(-1., -.5);
			glVertex2d(1., -.5);
			glVertex2d(1., -.7);
			glVertex2d(-1., -.7);
			glEnd();

			glRasterPos2d(- 0. / wd->vw->vp.w, -.4 + 16. * 4 / wd->vw->vp.h);
			gldPutString(cpplib::dstring(p->currentWarpDist / p->totalWarpDist * 100.) << '%');
			if(velo != 0){
				double eta = (p->totalWarpDist - p->currentWarpDist) / p->warpSpeed;
				eta = fabs(eta);
				if(3600 * 24 < eta)
					sprintf(buf, "ETA: %d days %02d:%02d:%02lg", (int)(eta / (3600 * 24)), (int)(fmod(eta, 3600 * 24) / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else if(60 < eta)
					sprintf(buf, "ETA: %02d:%02d:%02lg", (int)(eta / 3600), (int)(fmod(eta, 3600) / 60), fmod(eta, 60));
				else
					sprintf(buf, "ETA: %lg sec", eta);
				glRasterPos2d(- 8. / wd->vw->vp.w, -.4);
				gldPutString(buf);
			}
		}
	}

	glPopAttrib();
	glPopMatrix();
}

