/** \file
 * \brief Implementation of WarField and WarSpace's graphics.
 */
#include "Warpable.h"
#include "Player.h"
#include "Viewer.h"
#ifdef _WIN32
#include "glw/glwindow.h"
#include "../glw/GLWmenu.h"
#endif
#include "stellar_file.h"
#include "glstack.h"
#include "draw/WarDraw.h"
#include "glsl.h"
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#ifdef _WIN32
#include <clib/GL/gldraw.h>
#include <clib/GL/multitex.h>
#endif
}



int g_healthbar = 1;




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


void Warpable::drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale){
	Mat4d mat;
	transform(mat);
	double vsp = -mat.vec3(2).sp(velo) / getManeuve().maxspeed;
	if(1. < vsp)
		vsp = 1.;

	if(DBL_EPSILON < vsp){
		const Vec3d pos0 = nozzlepos + Vec3d(0,0, scale * vsp);
		class Tex1d{
		public:
			GLuint tex[2];
			Tex1d(){
				glGenTextures(2, tex);
			}
			~Tex1d(){
				glDeleteTextures(2, tex);
			}
		};
		static OpenGLState::weak_ptr<Tex1d> texname;
		glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		{
			Vec3d v = rot.cnj().trans(wd->vw->pos - mat.vp3(pos0));
			v[2] /= 2. * vsp;
			v.normin() *= (4. / 5.);

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGendv(GL_S, GL_OBJECT_PLANE, Vec4d(v));
			glEnable(GL_TEXTURE_GEN_S);
		}
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		if(!texname){
			texname.create(*openGLState);
			static const GLubyte texture[4][4] = {
				{255,0,0,0},
				{255,127,0,127},
				{255,255,0,191},
				{255,255,255,255},
			};
			glBindTexture(GL_TEXTURE_1D, texname->tex[0]);
			glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			static const GLubyte texture2[4][4] = {
				{255,127,64,63},
				{255,191,127,127},
				{255,255,255,191},
				{191,255,255,255},
			};
			glBindTexture(GL_TEXTURE_1D, texname->tex[1]);
			glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture2);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		if(!texname)
			return;
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, texname->tex[0]);
		if(glActiveTextureARB){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGendv(GL_S, GL_OBJECT_PLANE, Vec4d(0,0,-.5,.5));
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_1D);
			glBindTexture(GL_TEXTURE_1D, texname->tex[1]);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		glPushMatrix();
		glMultMatrixd(mat);
		gldTranslate3dv(pos0);
		glScaled(scale, scale, scale * 2. * (0. + vsp));
		glColor4f(1,1,1, vsp);
//		gldMultQuat(rot.cnj() * wd->vw->qrot.cnj());
		gldOctSphere(2);
		glPopMatrix();
		glPopAttrib();
	}
}

bool Frigate::cull(wardraw_t *wd){
	double pixels;
	if(wd->vw->gc->cullFrustum(this->pos, .6))
		return true;
	pixels = .8 * fabs(wd->vw->gc->scale(this->pos));
	if(pixels < 2)
		return true;
	return false;
}

int Frigate::popupMenu(PopupMenu &list){
	int ret = st::popupMenu(list);
	list.append("Dock", 0, "dock").append("Military Parade Formation", 0, "parade_formation").append("Cloak", 0, "cloak");
	return ret;
}

void Frigate::drawShield(wardraw_t *wd){
	Frigate *p = this;
	if(!wd->vw->gc->cullFrustum(pos, .1)){
		Mat4d mat;
		transform(mat);
		glColor4ub(255,255,9,255);
		glBegin(GL_LINES);
		glVertex3dv(mat.vp3(Vec3d(.001,0,0)));
		glVertex3dv(mat.vp3(Vec3d(-.001,0,0)));
		glEnd();
		{
			int i, n = 3;
			static const avec3_t pos0[] = {
				{0, 210 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{190 * BEAMER_SCALE, -120 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{-190 * BEAMER_SCALE, -120 * BEAMER_SCALE, 240 * BEAMER_SCALE},
				{0, .002 + 50 * BEAMER_SCALE, -60 * BEAMER_SCALE},
				{0, .002 + 60 * BEAMER_SCALE, 40 * BEAMER_SCALE},
				{0, -.002 + -50 * BEAMER_SCALE, -60 * BEAMER_SCALE},
				{0, -.002 + -60 * BEAMER_SCALE, 40 * BEAMER_SCALE},
			};
			avec3_t pos;
			double rad = .002;
			GLubyte col[4] = {255, 127, 127, 255};
			random_sequence rs;
			init_rseq(&rs, (unsigned long)this);
			if(fmod(wd->vw->viewtime + drseq(&rs) * 2., 2.) < .2){
				col[1] = col[2] = 255;
				n = numof(pos0);
				rad = .003;
			}
			for(i = 0 ; i < n; i++){
				mat4vp3(pos, mat, pos0[i]);
				gldSpriteGlow(pos, rad, col, wd->vw->irot);
			}
		}

		se.draw(wd, this, BEAMER_SHIELDRAD, p->shieldAmount / MAX_SHIELD_AMOUNT);
	}
}

