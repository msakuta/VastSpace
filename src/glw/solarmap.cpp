/** \file
 * \brief Solar system browser window definition.
 *
 * Additionally defines GLWinfo, a window displays information about specific astronomical object.
 */

#define NOMINMAX
#include "astrodraw.h"
#include "../galaxy_field.h"
#include "Player.h"
#include "glw/glwindow.h"
#include "GLWmenu.h"
#include "CoordSys.h"
#include "stellar_file.h"
#include "Entity.h"
#include "antiglut.h"
#include "cmd.h"
#include "astro_star.h"
#include "sqadapt.h"
#include "Game.h"
#include "EntityCommand.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
}
#include <algorithm>

#if 0
static int galaxy_map_slice = HFIELDZ;

static void draw_galaxy_map(struct coordsys *csys, Viewer *vw){
	GLubyte (*field)[FIELD][FIELDZ][4] = g_galaxy_field;
	{
		int xi, yi, zi;
		zi = galaxy_map_slice;
		for(xi = 0; xi < FIELD; xi++) for(yi = 0; yi < FIELD; yi++){
			glBegin(GL_QUADS);
			glColor4ubv(field[xi][yi][zi]);
			glVertex3d(xi, yi, -1);
			glVertex3d(xi+1, yi, -1);
			glVertex3d(xi+1, yi+1, -1);
			glVertex3d(xi, yi+1, -1);
			glEnd();
		}
	}
}

static int GalaxyMapShow(struct glwindow *wnd, int mx, int my, double gametime){
	extern struct player *ppl;
	extern struct coordsys *g_galaxysystem, *g_solarsystem;
	avec3_t plpos, v0, solarsystempos;
	int v1[3];
	int n = (ppl->pos[1] - 0.) / .005;
	tocs(solarsystempos, g_galaxysystem, avec3_000, g_solarsystem);
	tocs(plpos, g_galaxysystem, ppl->pos, ppl->cs);
	VECSCALE(v0, plpos, FIELD / GALAXY_EXTENT);
	VECSADD(v0, solarsystempos, 1. * FIELD);
	VECCPY(v1, v0);
	glPushMatrix();
	glTranslated(wnd->x, wnd->y + 12, 0);
/*	glScaled(wnd->w / MAZE_SIZE, wnd->w / MAZE_SIZE, 1.);*/
	draw_galaxy_map(g_galaxysystem, ppl);
	glBegin(GL_LINES);
	glColor4ub(255,255,255,255);
	glVertex3i(0, -v1[1], -1);
	glVertex3i(wnd->w, -v1[1], -1);
	glVertex3i(v1[0], 0, -1);
	glVertex3i(v1[0], wnd->h - 12, -1);
	glEnd();
	glPopMatrix();
	glRasterPos3d(wnd->x, wnd->y + 24, -1);
	glColor4ub(255,255,255,255);
	gldprintf("(%lg,%lg,%lg)", v0[0] , v0[1], v0[2]);
	return 0;
}

static int GalaxyMapMouse(struct glwindow *wnd, int mbutton, int state, int mx, int my){
/*	int y, x;
	extern struct player *ppl;
	int n = (ppl->pos[1] - 0.) / .005;
	char (*maze)[MAZE_SIZE];
	double oerad2;
	avec3_t plpos;
	tocs(plpos, g_earthsurface, ppl->pos, ppl->cs);
	oerad2 = (plpos[0] - orbelev_pos[0]) * (plpos[0] - orbelev_pos[0]) + (plpos[2] - orbelev_pos[2]) * (plpos[2] - orbelev_pos[2]);
	if(.1 * .1 < oerad2)
		return;
	maze = maze_initialize();
	if(!maze)
		return 0;
	x = my * MAZE_SIZE / wnd->w;
	y = mx * MAZE_SIZE / wnd->w;
	if(state == GLUT_KEEP_DOWN && 0 <= y && y < MAZE_SIZE && 0 <= x && x < MAZE_SIZE){
		maze[y][x] |= 4;
	}*/
	return 1;
}

static int GalaxyMapKey(struct glwindow *wnd, int key){ /* returns nonzero if processed */
	switch(key){
	case '+':
		if(galaxy_map_slice+1 < FIELDZ)
			galaxy_map_slice++;
		break;
	case '-':
		if(0 < galaxy_map_slice)
			galaxy_map_slice--;
		break;
	}
	return 0;
}

static glwindow *GalaxyMapShowWindow(){
	glwindow *ret;
	glwindow **ppwnd;
	int i;
	static const char *windowtitle = "Galaxy map";
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}
	ret = malloc(sizeof *ret);
	ret->x = 50;
	ret->y = 50;
	ret->w = FIELD+1;
	ret->h = FIELD+1 + 12;
	ret->title = windowtitle;
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE;
	ret->modal = NULL;
	ret->draw = GalaxyMapShow;
	ret->mouse = GalaxyMapMouse;
	ret->key = GalaxyMapKey;
	ret->destruct = NULL;
	glwActivate(glwAppend(ret));
	return ret;
}


int cmd_togglegalaxymap(int argc, const char *argv[]){
	GalaxyMapShowWindow();
	return 0;
}
#endif










































/*
entity_t **rstations = NULL;
int nrstations = 0;
*/

/// \brief Solar system browser window.
///
/// Displays the whole system in a window and enables the user to browse through.
/// Range and position of displayed space is adjustable with mouse and keys.
class GLwindowSolarMap : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLwindowSolarMap(Game *game, const char *title, Player *pl);
	static GLwindow *showWindow(Player *pl);
	const char *classname()const{return "GLwindowSolarMap";}
	void draw(GLwindowState &ws, double t);
//	int mouse(int button, int state, int x, int y);
	int key(int key);
	int specialKey(int key);
	int mouse(GLwindowState &ws, int button, int state, int x, int y);
	void anim(double dt);
	Vec3d org;
	Vec3d pointer;
	double lastt;
	double range;
	double dstrange;
	Quatd rot;
	int morg[2];
	int hold;
	const CoordSys *sol; ///< currently showing solar system
	struct teleport *target;
	const Astrobj *targeta; ///< astro
	Entity *targete; ///< entity
	WarField *targetc; ///< collapsed
	Entity *targetr; ///< resource station
	int focusc, sync; ///< Focus on current position
	struct teleport *focus;
	const Astrobj *focusa;
	Player *ppl;
protected:

	struct drawSolarMapItemParams;
	int drawSolarMapItem(drawSolarMapItemParams *);
	void drawMapCSOrbit(const CoordSys *vwcs, const CoordSys *cs, struct drawSolarMapItemParams *params);
private:
	void drawInt(const CoordSys *cs, drawSolarMapItemParams &params);
};

struct GLwindowSolarMap::drawSolarMapItemParams{
	double hitrad;
	double rad;
	Mat4d viewmat;
	Vec3d spointer; // pointer in space coordinates
	Vec3d apos0;
	Vec3d org;
	const CoordSys *sol;
	const char *name;
	int mousex;
	int mousey;
	int *iofs;
	Vec3d *pos;
	GLubyte pointcolor[4];
	GLubyte textcolor[4];
	int solid;
};

GLwindowSolarMap::GLwindowSolarMap(Game *game, const char *title, Player *apl) : st(game, title), ppl(apl), org(vec3_000), pointer(vec3_000), rot(quat_u){
	xpos = 50;
	ypos = 50;
	width = FIELD+1;
	height = FIELD+1 + 12;
	flags = GLW_CLOSE | GLW_COLLAPSABLE;
	modal = NULL;
	GLwindowSolarMap *p = this;
	p->lastt = 0.;
	p->range = p->dstrange = 1.;
	p->morg[0] = p->morg[1] = 0;
	p->hold = 0;
	p->focusc = 0;
	p->sync = 0;
	p->sol = NULL;
	p->target = NULL;
	p->targeta = NULL;
	p->targetc = NULL;
	p->targete = NULL;
	p->targetr = NULL;
	p->focus = NULL;
	p->focusa = NULL;
}

int GLwindowSolarMap::drawSolarMapItem(struct drawSolarMapItemParams *params){
	double hitrad = params->hitrad;
	GLwindowSolarMap *p = this;
	const CoordSys *sol = params->sol;
	const Vec3d &apos0 = params->apos0;
	const char *name = params->name;
	int *iofs = params->iofs;
	Vec3d &pos = *params->pos;

	GLwindow *wnd = p;
	double md;
	double h = this->height - 12;
	double range = sol->csrad * p->range;
	int ret = 0;

	Vec3d apos = p->rot.trans(apos0) + params->org * -sol->csrad;
	md = (apos[0] - params->spointer[0]) * (apos[0] - params->spointer[0]) + (apos[1] - params->spointer[1]) * (apos[1] - params->spointer[1]);
	glColor4ubv(params->pointcolor);
	if(4 * range / this->width < params->rad){
		double (*cuts)[2];
		int i;
		cuts = CircleCuts(16);
		glBegin(params->solid ? GL_POLYGON : GL_LINE_LOOP);
		for(i = 0; i < 16; i++){
			glVertex3d(apos[0] + params->rad * cuts[i][0], apos[1] + params->rad * cuts[i][1], apos[2]);
		}
		glEnd();
	}
	{
		glPointSize(GLfloat(md < hitrad * hitrad ? 5 : 3));
		glBegin(GL_POINTS);
		glVertex3dv(apos);
		glEnd();
	}
	if(md < hitrad * hitrad && params->name){
		double screeny;
		if(*iofs == 0){
/*			VECSADD(apos, org, -sol->rad);*/
			VECSCALE(pos, apos, 1. / range);
		}
		screeny = pos[1] + 2. * *iofs / h;
		glPushMatrix();
		glLoadIdentity();
		if((1. - screeny) * h / 2. - 10 < params->mousey && params->mousey < (1. - screeny) * h / 2. + 0){
			glColor4ub(0,0,255,128);
			glBegin(GL_QUADS);
			glVertex3d(pos[0], screeny, 0.);
			glVertex3d(pos[0] + 2. * (strlen(name) * 8) / this->width, screeny, 0.);
			glVertex3d(pos[0] + 2. * (strlen(name) * 8) / this->width, screeny + 2. * 10 / height, 0.);
			glVertex3d(pos[0], screeny + 2. * 10 / h, 0.);
			glEnd();
			ret = 1;
		}
		glColor4ubv(params->textcolor);
		glRasterPos3d(pos[0], pos[1] + 2. * *iofs / h, pos[2]);
/*				glRasterPos3d(pos[0] + vofs[0], pos[1] + vofs[1], pos[2] + vofs[2]);*/
		*iofs -= 10;
		gldprintf(name);
		glPopMatrix();
	}
	return ret;
}

void GLwindowSolarMap::drawMapCSOrbit(const CoordSys *vwcs, const CoordSys *cs, struct drawSolarMapItemParams *params){
	Vec3d &pos = *params->pos;
	CoordSys *cs2;
	if(!cs)
		return;
	for(cs2 = cs->children; cs2; cs2 = cs2->next)
		drawMapCSOrbit(vwcs, cs2, params);
	if(cs->w){
//		int i, viewstate;
		int collapse, plent = 0, plene = 0;
		Entity *pt;
		Mat4d mat, lmat;
		double rrange;
/*		viewstate = warf_viewstate(cs->w);*/
		mat = vwcs->tocsim(cs);
		mat4mp(lmat, params->viewmat, mat);
/*		for(i = 0; i < nrstations; i++) if(rstations[i]->w == cs->w){
			tocs(params->apos0, vwcs, rstations[i]->pos, cs);
			params->name = "Resource Station";
			params->rad = MAX(params->p->range * params->sol->rad * 20. / params->p->st.st.w, ((struct entity_private_static*)rstations[i]->vft)->getHitRadius);
			params->pointcolor[0] = rstations[i]->race == ppl->race ? 0 : 255;
			params->pointcolor[1] = rstations[i]->race != ppl->race && 0 <= rstations[i]->race ? 0 : 255;
			params->pointcolor[2] = 0;
			params->textcolor[0] = rstations[i]->race == ppl->race ? 0 : 255;
			params->textcolor[1] = rstations[i]->race != ppl->race && 0 <= rstations[i]->race ? 0 : 255;
			params->textcolor[2] = 0;
			if(drawSolarMapItem(params))
				params->p->targetr = rstations[i];
		}*/
		rrange = this->range * params->sol->csrad * 10. / this->width;
		collapse = cs->csrad < rrange;
		for(WarField::EntityList::iterator it = cs->w->el.begin(); it != cs->w->el.end(); it++){
			if(!*it)
				continue;
			Entity *pt = *it;
			if(strcmp(pt->classname(), "rstation") && (pt->race == ppl->race /*|| race_entity_visible(ppl->race, pt)*/)){
	//			avec3_t warpdst, pos1;
				int enter = 1;
				params->apos0 = vwcs->tocs(pt->pos, cs);
	/*			if(warpable_dest(pt, warpdst, vwcs)){
					glBegin(GL_LINES);
					mat4vp3(pos1, params->viewmat, params->apos0);
					glColor4ub(31,63,255,255);
					glVertex3dv(pos1);
					mat4vp3(pos1, params->viewmat, warpdst);
					glColor4ub(31,255,255,255);
					glVertex3dv(pos1);
					glEnd();
				}*/
				if(plent && ppl->race == pt->race && collapse)
					enter = 0;
				if(!plent && ppl->race == pt->race)
					plent = 1;
				if(plene && ppl->race != pt->race && collapse)
					enter = 0;
				if(!plene && ppl->race != pt->race)
					plene = 1;
				if(enter){
					params->rad = std::max(rrange, pt->getHitRadius());
					params->name = collapse ? ppl->race == pt->race ? "Your Units" : "Enemy Units" : pt->classname();
					params->pointcolor[0] = 255 * (ppl->race != pt->race);
					params->pointcolor[1] = 255 * (ppl->race == pt->race || pt->race < -1);
					params->pointcolor[2] = 255 * (ppl->race == pt->race);
					VECCPY(params->textcolor, params->pointcolor);
					if(drawSolarMapItem(params)){
						targete = pt;
						if(ppl->race == pt->race && collapse)
							targetc = cs->w;
					}
				}
			}
		}
	}
	if(cs->toOrbitCS() && cs->toOrbitCS()->flags2 & OCS_SHOWORBIT && cs->toOrbitCS()->orbit_home){
		const OrbitCS *a = cs->toOrbitCS();
		const Astrobj *home = a->orbit_home;

		// Obtain orbit center position
		Vec3d spos = vwcs->tocs(home->pos, home->parent);
		params->apos0 = vwcs->tocs(cs->pos, cs->parent);

		// Calculate local orbit matrix
		Mat4d qmat = a->orbit_axis.tomat4();
		double rad = a->orbit_rad;
		// Semi-minor axis
		double smia = rad * (a->eccentricity != 0. ? sqrt(1. - a->eccentricity * a->eccentricity) : 1.);
		qmat.scalein(rad, smia, rad);
		// Adjust for elliptical orbit
		if(a->eccentricity != 0.)
			qmat.translatein(a->eccentricity, 0, 0);
		Mat4d mat = vwcs->tocsim(home->parent);
		Mat4d rmat = mat * qmat;
		rmat.vec3(3) += spos;

		// Cache circle divisions.  Just 2 stage LOD would be enough.
		// Modern FPUs may not benefit from manual caching of repeated trigonometric values.
		// We may be better to care about cache hit ratio.
		const int divides = rad < range * sol->csrad * 100 ? 128 : 512;
		double (*cuts)[2] = CircleCuts(divides);

		// Obtain a matrix to convert local orbit coordinates to viewing coordinates.
		Mat4d lmat = params->viewmat * rmat;
		// Remember the vector for repeated scalar products.
		const Vec3d projVec = lmat.transpose().vec3(2);

		// Actually draw the (possibly flattened) ring.
		glBegin(GL_LINE_LOOP);
		for(int j = 0; j < divides; j++){
			Vec3d v(cuts[j][0], cuts[j][1], 0);

			// Determine whether the ring segment vector is pointing toward or away from the player.
			// lmat.transpose().dvp3(v) would yield the same result, but this one's more optimal.
			if(0 <= projVec.sp(v))
				glColor4f(1., 0.75, 1., 1.); // Draw the near side of the orbit with brighter color.
			else
				glColor4f(0.5, 0.75 * 0.5, 0.5, 1.); // The other side is darker.
			glVertex3dv(lmat.vp3(v));
		}
		glEnd();

		// Draw the orbit name
		params->name = cs->fullname ? cs->fullname : cs->name;
		params->rad = cs->csrad;
	}
	if((cs->flags & (CS_EXTENT | CS_ISOLATED)) == (CS_EXTENT | CS_ISOLATED) && 0 < cs->aorder.size()){
		Vec3d spos, apos;
		params->pointcolor[0] = 191;
		params->pointcolor[1] = 255;
		params->pointcolor[2] = 191;
		params->pointcolor[3] = 255;
		params->textcolor[0] = 191;
		params->textcolor[1] = 255;
		params->textcolor[2] = 191;
		params->textcolor[3] = 255;
		for(CoordSys::AOList::const_iterator i2 = cs->aorder.begin(); i2 != cs->aorder.end(); i2++){
			Astrobj *a2 = (*i2)->toAstrobj();
			if(!a2)
				continue;
			params->apos0 = vwcs->tocs(a2->pos, a2->parent);
#if 1
			params->name = a2->name;
			params->rad = a2->rad;
			if(drawSolarMapItem(params))
				targeta = a2;
#else
			md = (apos[0] - pointer[0]) * (apos[0] - pointer[0]) + (apos[1] - pointer[1]) * (apos[1] - pointer[1]);
			glPointSize(md < hitrad * hitrad ? 5 : 3);
			glColor4ub(191,255,191,255);
			glBegin(GL_POINTS);
			glVertex3dv(apos);
			glEnd();
			if(md < hitrad * hitrad){
				glColor4ub(255,255,191,255);
				if(*ofs == 0.)
					VECCPY(pos, apos);
				glRasterPos3d(pos[0], pos[1] + *ofs, pos[2]);
				*ofs -= hitrad * 1.5;
				gldprintf(a2->fullname ? a2->fullname : a2->name);
			}
#endif
		}
	}
}

static const double scalespace = 10, scalewidth = 10, scalerange = 10;
static const double scalevspace = 10 + 10;

void GLwindowSolarMap::draw(GLwindowState &ws, double gametime){
	int mousex = ws.mousex, mousey = ws.mousey;
	GLwindowSolarMap *p = this;
	Mat4d proj;
	Vec3d dst, pos;
	const CoordSys *sol;
	extern CoordSys *g_galaxysystem;
	GLWrect cr = clientRect();
	GLint vp[4], nvp[4];
	double dt = p->lastt ? gametime - p->lastt : 0.;
	int mi = MIN(this->width, this->height - 12) - 1;
	int ma = MAX(this->width, this->height - 12) - 1;
	double fx = (double)this->width / ma, fy = (double)(this->height - 12) / ma;

	{
		const CoordSys *cs;
		sol = NULL;
		for(cs = ppl->cs; cs; cs = cs->parent) if(cs->flags & CS_SOLAR){
			sol = cs;
			break;
		}
	}
/*	sol = findcs(g_galaxysystem, "sol");*/
	p->target = NULL;
	p->targeta = NULL;
	p->targete = NULL;
	p->targetc = NULL;
	p->targetr = NULL;
	p->sol = NULL;
	if(sol){
/*		extern Astrobj **astrobjs;
		extern int nastrobjs;*/
		int i, ip[2], h = this->height - 12, iofs = 0;
		double range = sol->csrad * p->range;
		double hitrad = sol->csrad * .05 * p->range * (this->height - 12) / ma;
		Vec3d pos(vec3_000);
		Vec3d plpos;
		struct drawSolarMapItemParams params;
		timemeas_t tm;
		TimeMeasStart(&tm);
		p->sol = sol;

		params.hitrad = hitrad;
		params.sol = sol;
		params.mousex = mousex;
		params.mousey = mousey;
		params.apos0;
		params.name;
		params.iofs = &iofs;
		params.pos = &pos;
		params.solid = 0;

		if(p->focusa){
			Vec3d focuspos = sol->tocs(p->focusa->pos, p->focusa->parent);
			p->org = focuspos * 1. / sol->csrad;
		}
		else if(p->focus){
			Vec3d focuspos = sol->tocs(p->focus->pos, p->focus->cs);
			p->org = focuspos * 1. / sol->csrad;
		}
		else if(p->focusc){
			p->org = sol->tocs(ppl->getpos(), ppl->cs);
			p->org *= 1. / sol->csrad;
		}
		if(p->sync){
			p->rot = ppl->getrot();
		}
		params.viewmat = p->rot.tomat4();
		params.viewmat.translatein(-p->org[0] * sol->csrad, -p->org[1] * sol->csrad, -p->org[2] * sol->csrad);

		if(this){
			RECT rc;
			GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
			glGetIntegerv(GL_VIEWPORT, vp);
			glViewport(nvp[0] = this->xpos + 1, nvp[1] = rc.bottom - rc.top - (this->ypos + this->height) + 1, nvp[2] = this->width - 1, nvp[3] = this->height - 12 - 1);
		}

		// Set up projection matrix for solar map
		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		glLoadIdentity();
		// Set depth of orthogonal projection 100 times the radius of the solar system.
		// The system may contain objects that exceeds declared radius.
		glOrtho(-fx, fx, -fy, fy, -100. / p->range, 100. / p->range);
		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glLoadIdentity();
/*		gldMultQuat(p->rot);
		glTranslated(-p->org[0] / p->range, -p->org[1] / p->range, -p->org[2] / p->range);*/
		gldScaled(1. / range);
		if(p->hold/* && p->pointer[0] - 10 <= mousex && mousex < p->pointer[0] + 100 && p->pointer[1] - 10 <= mousey && mousey < p->pointer[1] + 100*/){
			ip[0] = int(p->pointer[0]);
			ip[1] = int(p->pointer[1]);
		}
		else{
			ip[0] = mousex;
			ip[1] = mousey;
			p->hold = 0;
		}
		params.spointer[0] = (2 * range * ip[0] / cr.width() - range) * fx;
		params.spointer[1] = (-2 * range * ip[1] / cr.height() + range) * fy;
		params.spointer[2] = 0.;
		params.org = p->rot.trans(p->org);
//		params.spointer += org * sol->csrad;
		glPushAttrib(GL_POINT_BIT);
		glDisable(GL_POINT_SMOOTH);
#if 0
		{
			struct hypersonar *hs;
/*			struct drawSolarMapItemParams *params = params;*/
			for(hs = g_hsonar; hs; hs = hs->next){
				double f = hs->life < 0. ? (10. + hs->life) / 10. : 1.;
				tocs(params.apos0, sol, hs->pos, hs->cs);
				params.name = NULL;
				params.rad = hs->rad;
				params.pointcolor[0] = !!hs->type * 255;
				params.pointcolor[1] = 0;
				params.pointcolor[2] = 255;
				params.pointcolor[3] = 255 * f;
				params.textcolor[0] = !!hs->type * 255;
				params.textcolor[1] = 0;
				params.textcolor[2] = 255;
				drawSolarMapItem(&params);
				params.rad = hs->rad / 2.;
				params.solid = 1;
				params.pointcolor[3] = 64 * f;
				drawSolarMapItem(&params);
				params.solid = 0;
			}
		}
#endif
		params.pointcolor[0] = 191;
		params.pointcolor[1] = 255;
		params.pointcolor[2] = 255;
		params.pointcolor[3] = 255;
		params.textcolor[0] = 191;
		params.textcolor[1] = 255;
		params.textcolor[2] = 255;
		params.textcolor[3] = 255;
		for(Player::teleport_iterator it = 0; it != ppl->endTeleport(); it++){
/*			double md;
			avec3_t spos, apos, apos0;*/
			teleport *tp = ppl->getTeleport(it);
			params.apos0 = sol->tocs(tp->pos, tp->cs);
#if 1
			params.name = tp->name;
			params.rad = 0.;
			if(drawSolarMapItem(&params))
				p->target = tp;
#else
			quatrot(apos, p->rot, apos0);
			md = (apos[0] - pointer[0]) * (apos[0] - pointer[0]) + (apos[1] - pointer[1]) * (apos[1] - pointer[1]);
			glPointSize(md < hitrad * hitrad ? 5 : 3);
			glColor4ub(191,255,255,255);
			glBegin(GL_POINTS);
			glVertex3dv(apos0);
			glEnd();
			if(md < hitrad * hitrad){
				avec3_t vofs, vofs0 = {0};
				double screeny;
				vofs0[1] = ofs;
				quatirot(vofs, p->rot, vofs0);
				if(iofs == 0){
					VECSADD(apos, org, -sol->rad);
					VECSCALE(pos, apos, 1. / range);
				}
				screeny = pos[1] + 2. * iofs / h;
				glPushMatrix();
				glLoadIdentity();
				if((1. - screeny) * h / 2. - 10 < mousey && mousey < (1. - screeny) * h / 2. + 0){
					glColor4ub(0,0,255,128);
					glBegin(GL_QUADS);
					glVertex3d(pos[0], screeny, 0.);
					glVertex3d(pos[0] + 2. * (strlen(tplist[i].name) * 8) / wnd->w, screeny, 0.);
					glVertex3d(pos[0] + 2. * (strlen(tplist[i].name) * 8) / wnd->w, screeny + 2. * 10 / h, 0.);
					glVertex3d(pos[0], screeny + 2. * 10 / h, 0.);
					glEnd();
				}
				glColor4ub(191,255,255,255);
				glRasterPos3d(pos[0], pos[1] + 2. * iofs / h, pos[2]);
/*				glRasterPos3d(pos[0] + vofs[0], pos[1] + vofs[1], pos[2] + vofs[2]);*/
				ofs -= hitrad * 1.5;
				iofs -= 10;
				gldprintf(tplist[i].name);
				glPopMatrix();
			}
#endif
		}
/*		printf("sm-tp %lg\n", TimeMeasLap(&tm));*/
		drawMapCSOrbit(sol, sol, &params);
		params.pointcolor[0] = 191;
		params.pointcolor[1] = 191;
		params.pointcolor[2] = 127;
		params.pointcolor[3] = 255;
		params.textcolor[0] = 255;
		params.textcolor[1] = 191;
		params.textcolor[2] = 127;
		params.textcolor[3] = 255;
		drawInt(sol, params);
		if(p->hold && !(p->pointer[0] - 10 <= mousex && mousex < p->pointer[0] + 100 && p->pointer[1] - 10 <= mousey && mousey < p->pointer[1] - iofs + 10))
			p->hold = 0;
		if(!p->hold && iofs < 0){
			p->pointer[0] = mousex;
			p->pointer[1] = mousey;
			p->hold = 1;
		}
		glPopAttrib();
		glPopMatrix();
		{
			Vec3d plpos0 = sol->tocs(ppl->getpos(), ppl->cs);
			plpos0 += p->org * -sol->csrad;
			plpos0 *= 1. / range * this->width;
			plpos = p->rot.trans(plpos0);
		}
		glPushMatrix();
		glLoadIdentity();
		glScaled(1. / this->width, 1. / this->width, 1. / this->width);	glBegin(GL_LINES);
		glColor4ub(127,255,127,255);
		glBegin(GL_LINES);
		glVertex3d(plpos[0] + 8, plpos[1] + 8, 0);
		glVertex3d(plpos[0] - 8, plpos[1] - 8, 0);
		glVertex3d(plpos[0] + 8, plpos[1] - 8, 0);
		glVertex3d(plpos[0] - 8, plpos[1] + 8, 0);
		glEnd();
		if(p->focus || p->focusa){
			int ind = 1;
			Vec3d apos0 = sol->tocs(p->focus ? p->focus->pos : p->focusa->pos, p->focus ? p->focus->cs : p->focusa->parent);
			apos0 += p->org * -sol->csrad;
			apos0 *= 1. / range * this->width;
			Vec3d apos = p->rot.trans(apos0);
			glColor4ub(255,255,127,255);
			glBegin(GL_LINE_LOOP);
			glVertex2d(apos[0] - 8, apos[1] - 8);
			glVertex2d(apos[0] - 8, apos[1] + 8);
			glVertex2d(apos[0] + 8, apos[1] + 8);
			glVertex2d(apos[0] + 8, apos[1] - 8);
			glEnd();
		}

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(proj);
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		if(this){
			glViewport(vp[0], vp[1], vp[2], vp[3]);
		}

		{
			int i, y;
			int x = int(cr.x0 + cr.width() - scalespace);

			if(cr.x1 - scalespace - scalewidth <= ws.mx && ws.mx <= cr.x1 - scalespace && cr.y0 + scalevspace <= ws.my && ws.my <= cr.y1 - scalevspace){
				glColor4ub(0,63,127,127);
				glBegin(GL_QUADS);
				glVertex2d(x, cr.y0 + scalevspace);
				glVertex2d(x - scalewidth, cr.y0 + scalevspace);
				glVertex2d(x - scalewidth, cr.y1 - scalevspace);
				glVertex2d(x, cr.y1 - scalevspace);
				glEnd();
			}

			glColor4ub(127,127,127,255);
			glBegin(GL_LINES);
			glVertex2d(x - scalewidth / 2, cr.y0 + scalevspace);
			glVertex2d(x - scalewidth / 2, cr.y0 + cr.height() - scalevspace);
			for(i = 0; i <= scalerange; i++){
				y = int(cr.y0 + scalevspace + (cr.height() - scalevspace * 2) * i / scalerange);
				glVertex2d(x - scalewidth, y);
				glVertex2d(x, y);
			}
			y = int(cr.y0 + scalevspace + (cr.height() - scalevspace * 2) * (scalerange + log10(p->range)) / scalerange);
			glEnd();
			glColor4ub(127,255,127,255);
			glBegin(GL_QUADS);
			glVertex2d(x, y - 3);
			glVertex2d(x - scalewidth, y - 3);
			glVertex2d(x - scalewidth, y + 3);
			glVertex2d(x, y + 3);
			glEnd();
			glwpos2d(x - scalewidth / 2 - 8 / 2, cr.y0 + scalevspace + scalewidth - 10);
			glwprintf("+");
			glwpos2d(x - scalewidth / 2 - 8 / 2, cr.y0 + cr.height() - scalevspace - scalewidth + 20);
			glwprintf("-");
		}

		glColor4ub(255,255,255,255);
		glwpos2d(cr.x0 + 2, cr.y0 + cr.height() - 12);
		glwprintf("%s", sol->fullname ? sol->fullname : sol->name);
		glwpos2d(cr.x0 + 2, cr.y0 + cr.height() - 2);
		glwprintf("Range: %lg km", range);

		if(0 <= mousex && mousex < 48 && 0 <= mousey && mousey <= 12){
			int ind = mousex / 12;
			glColor4ub(191,31,31,127);
			glBegin(GL_QUADS);
			glVertex2d(cr.x0 + ind * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0 + 12);
			glVertex2d(cr.x0 + (ind) * 12, cr.y0 + 12);
			glEnd();
			glColor4ub(191,255,255,255);
			glwpos2d(cr.x0 + 48 + 2, cr.y0 + 12);
			glwprintf(ind == 0 ? "Reset View" : ind == 1 ? "Defocus" : ind == 2 ? "Current Position" : "Synchronize Rotation");
		}
		if(p->focus || p->focusa){
			int ind = 1;
			glColor4ub(31,127,127,127);
			glBegin(GL_QUADS);
			glVertex2d(cr.x0 + ind * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0 + 12);
			glVertex2d(cr.x0 + (ind) * 12, cr.y0 + 12);
			glEnd();
		}
		if(p->focusc){
			int ind = 2;
			glColor4ub(31,127,127,127);
			glBegin(GL_QUADS);
			glVertex2d(cr.x0 + ind * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0 + 12);
			glVertex2d(cr.x0 + (ind) * 12, cr.y0 + 12);
			glEnd();
		}
		if(p->sync){
			int ind = 3;
			glColor4ub(31,127,127,127);
			glBegin(GL_QUADS);
			glVertex2d(cr.x0 + ind * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0);
			glVertex2d(cr.x0 + (ind + 1) * 12, cr.y0 + 12);
			glVertex2d(cr.x0 + (ind) * 12, cr.y0 + 12);
			glEnd();
		}
		glColor4ub(191,255,255,255);
		glwpos2d(cr.x0 + 1, cr.y0 + (1) * 12);
		glwprintf("R");
		glwpos2d(cr.x0 + 1 + 12, cr.y0 + (1) * 12);
		glwprintf("F");
		glwpos2d(cr.x0 + 1 + 24, cr.y0 + (1) * 12);
		glwprintf("C");
		glwpos2d(cr.x0 + 1 + 36, cr.y0 + (1) * 12);
		glwprintf("S");
		glBegin(GL_LINES);
		glVertex2d(cr.x0, cr.y0 + 12);
		glVertex2d(cr.x0 + 48, cr.y0 + 12);
		glVertex2d(cr.x0 + 12, cr.y0 + 12);
		glVertex2d(cr.x0 + 12, cr.y0);
		glVertex2d(cr.x0 + 24, cr.y0 + 12);
		glVertex2d(cr.x0 + 24, cr.y0);
		glVertex2d(cr.x0 + 36, cr.y0 + 12);
		glVertex2d(cr.x0 + 36, cr.y0);
		glVertex2d(cr.x0 + 48, cr.y0 + 12);
		glVertex2d(cr.x0 + 48, cr.y0);
		glEnd();
/*		printf("sm-all %lg\n", TimeMeasLap(&tm));*/
	}
}

void GLwindowSolarMap::drawInt(const CoordSys *cs, drawSolarMapItemParams &params){
	for(CoordSys::AOList::const_iterator i = cs->aorder.begin(); i != cs->aorder.end(); i++){
		const OrbitCS *a = (*i)->toOrbitCS();
		if(!a)
			continue;
		int j;
		double (*cuts)[2], rad;
		const Astrobj *home = a->orbit_home;
		Vec3d spos, apos, apos0;
		Mat4d mat, qmat, rmat, lmat;
		Quatd q;
		double smia;
		if(sol != a->parent && !sol->is_ancestor_of(a->parent))
			continue;
		params.apos0 = sol->tocs(a->pos, a->parent);

#if 1
		params.name = a->fullname ? a->fullname : a->name;
		params.rad = a->csrad;
		if(drawSolarMapItem(&params))
			this->targeta = a->toAstrobj();
#else
		quatrot(apos, p->rot, apos0);
		md = (apos[0] - pointer[0]) * (apos[0] - pointer[0]) + (apos[1] - pointer[1]) * (apos[1] - pointer[1]);
		glPointSize(md < hitrad * hitrad ? 5 : 3);
		glColor4ub(191,255,191,255);
		glBegin(GL_POINTS);
		glVertex3dv(apos0);
		glEnd();
		if(md < hitrad * hitrad){
			avec3_t vofs, vofs0 = {0};
			double screeny;
			vofs0[1] = ofs;
			quatirot(vofs, p->rot, vofs0);
			glColor4ub(255,255,191,255);
			if(iofs == 0){
				VECSADD(apos, org, -sol->rad);
				VECSCALE(pos, apos, 1. / range);
			}
			screeny = pos[1] + 2. * iofs / h;
			glPushMatrix();
			glLoadIdentity();
			glRasterPos3d(pos[0], pos[1] + 2. * iofs / h, pos[2]);
/*				glRasterPos3d(pos[0] + vofs[0], pos[1] + vofs[1], pos[2] + vofs[2]);*/
			ofs -= hitrad * 1.5;
			iofs -= 10;
			gldprintf(a->name);
			glPopMatrix();
		}
#endif
/*		glBegin(GL_LINES);
		glVertex3dv(pointer);
		glVertex3dv(apos);
		glEnd();*/
	}
}

#if 0
HMENU hMapPopupMenu = NULL;
int entity_popup(entity_t *pt, int);
#endif

int GLwindowSolarMap::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWrect cr = clientRect();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
/*	if(my < 12)
		return 1;*/
	if(0 <= my && my <= 12 && 0 <= mx && mx < 48){
		int ind = mx / 12;
		if(state == GLUT_DOWN && mbutton == GLUT_LEFT_BUTTON) switch(ind){
			case 0: org.clear(); rot = quat_u; return 1;
			case 2: focusc = !focusc; /* fall through */
			case 1: focus = NULL; focusa = NULL; return 1;
			case 3: sync = !sync; return 1;
		}
		return 1;
	}
	if(cr.width() - scalespace - scalewidth <= mx && mx <= cr.width() - scalespace && scalevspace <= my && my <= cr.height() - scalevspace){
		if((state == GLUT_KEEP_DOWN || state == GLUT_DOWN) && mbutton == GLUT_LEFT_BUTTON)
			dstrange = pow(10, (my - scalevspace) * scalerange / (cr.height() - scalevspace * 2) - scalerange);
		return 1;
	}

	// Unit selection over solarmap
	if(state == GLUT_UP && (mbutton == GLUT_LEFT_BUTTON || mbutton == GLUT_RIGHT_BUTTON) && targete && hold != 2){
		if(targetc){
//			ppl->selected = targetc->el;
			for(WarField::EntityList::iterator it = targetc->el.begin(); it != targetc->el.end(); it++) if(*it){
				ppl->selected.insert(*it);
//				pt->selectnext = pt->next;
			}
		}
		else{
			ppl->selected.insert(targete);
//			ppl->selected = targete;
//			targete->selectnext = NULL;
		}
		if(mbutton != GLUT_LEFT_BUTTON){
			entity_popup(ppl->selected, ws, 1);
		}
		hold = 2;
		pointer[0] = pointer[1] = 0;
	}

	// Context menu over astronomic objects
	if(state == GLUT_UP && mbutton == GLUT_LEFT_BUTTON){
		if((target || targeta || targetr) && hold != 2){
//			int cmd_teleport(int argc, char *argv[]), cmd_warp(int argc, char *argv[]);
//			char *argv[3];
//			char buf[256];
			const char *name = target ? target->name : targeta ? targeta->name : targetr->classname();
			const char *typestring = target ? "Teleport" : "Astro";
			char titles0[5][128], *titles[5];
			int keys[5];
			char cmds0[5][128], *cmds[5];
			int i = 0, j = 0;
			GLwindow *glw;
			extern int s_mousex, s_mousey;

#if 1
			PopupMenu menu;

			struct PopupMenuItemFocus : public PopupMenuItem{
				GLwindowSolarMap *parent;
				const Astrobj *target;
				PopupMenuItemFocus(GLwindowSolarMap *parent, const Astrobj *target) : PopupMenuItem("Focus"), parent(parent), target(target){}
				virtual void execute(){
					parent->focusa = target;
				}
				virtual PopupMenuItem *clone()const{return new PopupMenuItemFocus(*this);}
			};
			if(targeta)
				menu.append(new PopupMenuItemFocus(this, targeta));

			struct PopupMenuItemWarp : public PopupMenuItem{
				GLwindowSolarMap *parent;
				teleport *target;
				PopupMenuItemWarp(GLwindowSolarMap *parent, teleport *target) : PopupMenuItem("Warp"), parent(parent), target(target){}
				virtual void execute(){
					Player::SelectSet &selected = parent->game->player->selected;
					WarpCommand com;
					com.destpos = target->pos;
					com.destcs = target->cs;
					for(Player::SelectSet::iterator it = selected.begin(); it != selected.end(); ++it) if(*it){
						(*it)->command(&com);
						CMEntityCommand::s.send(*it, com);
					}
				}
				virtual PopupMenuItem *clone()const{return new PopupMenuItemWarp(*this);}
			};
			if(target)
				menu.append(new PopupMenuItemWarp(this, target));

			GLwindow *glwInfo(const CoordSys *cs, int type, const char *name);
			struct PopupMenuItemInfo : public PopupMenuItem{
				GLwindowSolarMap *parent;
				const Astrobj *target;
				PopupMenuItemInfo(GLwindowSolarMap *parent, const Astrobj *target) : PopupMenuItem("Information"), parent(parent), target(target){}
				virtual void execute(){
					glwInfo(target, 0, target->name);
				}
				virtual PopupMenuItem *clone()const{return new PopupMenuItemInfo(*this);}
			};
			if(targeta)
				menu.append(new PopupMenuItemInfo(this, targeta));

			glw = glwPopupMenu(game, ws, menu);

#else
//			sprintf(titles[j] = titles0[i], "Focus");
			titles[j] = "Focus";
			keys[j] = 0;
			sprintf(cmds[j] = cmds0[i], "focus \"%s\"", name);
			i++; j++;
			if(target && target->flags & TELEPORT_TP){
				sprintf(titles[j] = titles0[i], "Teleport");
				keys[j] = 0;
				sprintf(cmds[j] = cmds0[i], "teleport \"%s\"", name);
				i++; j++;
			}

			if(target && target->flags & TELEPORT_WARP || targetr){
				sprintf(titles[j] = titles0[i], "Warp");
				keys[j] = 0;
				if(targetr)
					sprintf(cmds[j] = cmds0[i], "warp \"%s\" %lg %lg %lg", targetr->w->cs->name, targetr->pos[0], targetr->pos[1] + 1., targetr->pos[2]);
				else
					sprintf(cmds[j] = cmds0[i], "warp \"%s\"", name);
				i++; j++;
			}

			sprintf(titles[j] = titles0[i], "Information");
			keys[j] = 0;
			sprintf(cmds[j] = cmds0[i], "info %s \"%s\"", typestring, name);
			i++; j++;

//			glw = glwPopupMenu(ws, j, titles, keys, cmds, 0);
//			glw->x = s_mousex;
//			glw->y = s_mousey;
#endif

			hold = 2;
			pointer[0] = pointer[1] = 0;
		}
	}

	if(state == GLUT_DOWN && mbutton != GLUT_WHEEL_UP && mbutton != GLUT_WHEEL_DOWN){
		morg[0] = mx;
		morg[1] = my;
	}
	else if(mbutton == GLUT_RIGHT_BUTTON && (state == GLUT_UP || state == GLUT_KEEP_DOWN)){
		Quatd q, q1, q2;
		q1[0] = 0.;
		q1[1] = sin((mx - morg[0]) * M_PI * .002 * .5);
		q1[2] = 0.;
		q1[3] = cos((mx - morg[0]) * M_PI * .002 * .5);
		q2 = this->rot.itrans(q1);
		q2[3] = q1[3];
		q = this->rot * q2;
		q1[0] = sin((my - morg[1]) * M_PI * .002 * .5);
		q1[1] = 0.;
		q1[3] = cos((my - morg[1]) * M_PI * .002 * .5);
		q2 = this->rot.itrans(q1);
		q2[3] = q1[3];
		this->rot = q * q2;
		this->rot.normin();
		morg[0] = mx;
		morg[1] = my;
	}
#if 1
	else if(mbutton == GLUT_LEFT_BUTTON && (state == GLUT_UP || state == GLUT_KEEP_DOWN)){
/*		if(r_maprot){
			amat4_t rot;
			avec3_t pyr;
			double angle;
			double dx, dy;
			getrot(rot);
			imat2pyr(rot, pyr);
			angle = pyr[1];
			dx = 2. * p->range * -(mx - p->morg[0]) / wnd->w;
			dy = 2. * p->range * -(my - p->morg[1]) / wnd->w;
			p->org[0] += dx * cos(angle) + dy * -sin(angle);
			p->org[1] += dx * sin(angle) + dy * cos(angle);
		}
		else*/{
			Vec3d dr, dr0;
			dr0[0] = 2. * range * -(mx - morg[0]) / this->width;
			dr0[1] = 2. * range * (my - morg[1]) / this->width;
			dr0[2] = 0.;
			dr = this->rot.itrans(dr0);
			this->org += dr;
		}
		morg[0] = mx;
		morg[1] = my;
	}
#endif
	else if(mbutton == GLUT_WHEEL_UP)
		dstrange /= 2.;
	else if(mbutton == GLUT_WHEEL_DOWN)
		dstrange *= 2.;
	return 1;
}


int GLwindowSolarMap::key(int key){
	GLwindowSolarMap *p = this;
	switch(key){
		case 'r': VECNULL(p->org); QUATIDENTITY(p->rot); break;
		case 'c': p->focusc = !p->focusc; /* fall through */
		case 'f': p->focus = NULL; p->focusa = NULL; break;
		case 's': p->sync = !p->sync; break;
		case '+': p->dstrange /= 2.; break;
		case '-': p->dstrange *= 2.; break;
		case '[': width *= 2, height = width + 12; break;
		case ']': width /= 2, height = width + 12; break;
	}
	return 1;
}

int GLwindowSolarMap::specialKey(int key){
	switch(key){
		case GLUT_KEY_PAGE_UP: dstrange /= 2.; break;
		case GLUT_KEY_PAGE_DOWN: dstrange *= 2.; break;
	}
	return 1;
}

void GLwindowSolarMap::anim(double dt){
	double f = 1. - exp(-dt / .10);
	double lrange = log(range);
	double ldstrange = log(dstrange);
	lrange += (ldstrange - lrange) * f;
	range = exp(lrange);
}


static SQInteger sqf_GLwindowSolarMap_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	GLwindowSolarMap *p = new GLwindowSolarMap(game, "Solarsystem browser", game->player);
	GLelement::sq_assignobj(v, p);
	glwAppend(p);
	return 0;
}


static bool sq_GLwindowSolarMap_define(HSQUIRRELVM v){
	// Define class GLwindowSolarMap
	GLwindow::sq_define(v);
	sq_pushstring(v, _SC("GLwindowSolarMap"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLwindowSolarMap");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_GLwindowSolarMap_constructor);
/*	register_closure(v, _SC("addButton"), sqf_GLWbuttonMatrix_addButton);
	register_closure(v, _SC("addToggleButton"), sqf_GLWbuttonMatrix_addToggleButton);
	register_closure(v, _SC("addMoveOrderButton"), sqf_GLWbuttonMatrix_addMoveOrderButton);
	register_closure(v, _SC("addControlButton"), sqf_GLWbuttonMatrix_addControlButton);*/
	sq_createslot(v, -3);
	return true;
}

static sqa::Initializer init_GLWbuttonMatrix("GLwindowSolarMap", sq_GLwindowSolarMap_define);







































#if 1
/// \brief A window that tells information about certain celestial body.
class GLWinfo : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	int type;
	struct teleport *tp;
	Astrobj *a;
	GLWinfo(Game *game, const char *title) : st(game, title){}
	virtual void draw(GLwindowState &ws, double t);
	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y);
};

void GLWinfo::draw(GLwindowState &ws, double t){
	int mx = ws.mx, my = ws.my;
	GLWinfo *p = this;
	GLWinfo *wnd = this;
	GLWrect cr = clientRect();
	int iy = 0;
	glColor4ub(255,213,213,255);
	if(p->type == 1){
		if(!p->a)
			return;
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Astronomical Object");
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Name: %s", p->a->name);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("CoordSys: %s", p->a->parent->name);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Position: %lg,%lg,%lg", p->a->pos[0], p->a->pos[1], p->a->pos[2]);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Radius: %lg km", p->a->rad);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Mass: %lg kg", p->a->mass);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Flags: ");
		if(p->a->flags & AO_PLANET)
			glwprintf("Planet ");
		if(p->a->flags & AO_GRAVITY)
			glwprintf("Gravity ");
		if(p->a->flags & AO_SPHEREHIT)
			glwprintf("SphereHit ");
		if(p->a->flags & AO_PLANET && p->a->orbit_home){
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
			glwprintf("Orbits %s", p->a->orbit_home->name);
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
			glwprintf("Semi-major axis: %lg km", p->a->orbit_rad);
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
			glwprintf("Eccentricity: %lg", p->a->eccentricity);
		}
		if(&p->a->getStatic() == &Star::classRegister){
			Star *star = (Star*)p->a;
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
			Star::SpectralType spect = star->spect;
			if(spect == Star::Unknown)
				glwprintf("Spectral Type: Unknown");
			else
				glwprintf("Spectral Type: %s%g", Star::spectralToName(spect), star->subspect);
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
			glwprintf("Absolute Magnitude: %g", star->absmag);
		}
		for(int i = 0; i < p->a->extranames.size(); i++){
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
			glwprintf("Alias Name: %s", (const char*)p->a->extranames[i]);
		}
	}
	else if(p->type == 3){
		if(!p->tp)
			return;
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Teleport Site");
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Name: %s", p->tp->name);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("CoordSys: %s", p->tp->cs->name);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Position: %lg,%lg,%lg", p->tp->pos[0], p->tp->pos[1], p->tp->pos[2]);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * 12);
		glwprintf("Flags: ");
		if(p->tp->flags & TELEPORT_TP)
			glwprintf("Teleportable ");
		if(p->tp->flags & TELEPORT_WARP)
			glwprintf("Warpable ");
	}
}

int GLWinfo::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	return 0;
}

GLwindow *glwInfo(const CoordSys *cs, int type, const char *name){
	GLWinfo *ret = new GLWinfo(cs->getGame(), cpplib::dstring(name) << " Property");
	ret->setExtent(GLWrect(75, 75, 75 + 260, 75 + 2 * 12 + 10 * 12));
	ret->setClosable(true);
	ret->setCollapsable(true);
//	ret->modal = NULL;
//	glwsizeable_init(&p->st);
	glwAppend(ret);
	ret->type = type;
	ret->tp = NULL;
	ret->a = NULL;
/*	if(type == 0 || type == 3){
		for(int i = 0; i < ntplist; i++) if(!strcmp(tplist[i].name, name)){
			ret->tp = &tplist[i];
			if(type == 0)
				ret->type = 3;
			break;
		}
	}*/
	if(type == 0 || type == 1){
		Astrobj *ao = const_cast<CoordSys*>(cs)->findastrobj(name);
		if(ao){
			ret->a = ao;
			if(type == 0)
				ret->type = 1;
			return ret;
		}
	}
	if(ret->tp == NULL && ret->a == NULL){
		ret->postClose();
	}
	return ret;
}


#endif