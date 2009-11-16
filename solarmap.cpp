#include "astrodraw.h"
#include "galaxy_field.h"
#include "player.h"
#include "glwindow.h"
#include "coordsys.h"
#include "stellar_file.h"
#include "entity.h"
#include "antiglut.h"
//#include "scarry.h"
//#include "spacewar.h"
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
}
#include <algorithm>

// Windows header file defines "max" as a macro, which collides with std::max.
#ifdef max
#undef max
#endif

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
class GLwindowSolarMap : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;
	GLwindowSolarMap(const char *title, Player *pl);
	static GLwindow *showWindow(Player *pl);
	void draw(GLwindowState &ws, double t);
//	int mouse(int button, int state, int x, int y);
	int key(int key);
	int mouse(int button, int state, int x, int y);
	void anim(double dt);
protected:
	Vec3d org;
	Vec3d pointer;
	double lastt;
	double range;
	double dstrange;
	Quatd rot;
	int morg[2];
	int hold;
	const CoordSys *sol; /* currently showing solar system */
	struct teleport *target;
	const Astrobj *targeta; /* astro */
	Entity *targete; /* entity */
	WarField *targetc; /* collapsed */
	Entity *targetr; /* resource station */
	int focusc, sync; /* Focus on current position */
	struct teleport *focus;
	Astrobj *focusa;
	Player *ppl;

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

GLwindowSolarMap::GLwindowSolarMap(const char *title, Player *apl) : st(title), ppl(apl), org(vec3_000), pointer(vec3_000), rot(quat_u){
	x = 50;
	y = 50;
	w = FIELD+1;
	h = FIELD+1 + 12;
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
	const double *apos0 = params->apos0;
	const char *name = params->name;
	int *iofs = params->iofs;
	Vec3d &pos = *params->pos;

	GLwindow *wnd = p;
	double md;
	double h = this->h - 12;
	double range = sol->csrad * p->range;
	int ret = 0;

	Vec3d apos = p->rot.trans(apos0);
	md = (apos[0] - params->spointer[0]) * (apos[0] - params->spointer[0]) + (apos[1] - params->spointer[1]) * (apos[1] - params->spointer[1]);
	glColor4ubv(params->pointcolor);
	apos += params->org * -sol->csrad;
	if(4 * range / this->w < params->rad){
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
		glPointSize(md < hitrad * hitrad ? 5 : 3);
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
			glVertex3d(pos[0] + 2. * (strlen(name) * 8) / this->w, screeny, 0.);
			glVertex3d(pos[0] + 2. * (strlen(name) * 8) / this->w, screeny + 2. * 10 / h, 0.);
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
		int i, viewstate, collapse, plent = 0, plene = 0;
		Entity *pt;
		Mat4d mat, lmat;
		double rrange;
/*		viewstate = warf_viewstate(cs->w);*/
		mat = vwcs->tocsim(cs);
		mat4mp(lmat, params->viewmat, mat);
/*		for(i = 0; i < nrstations; i++) if(rstations[i]->w == cs->w){
			tocs(params->apos0, vwcs, rstations[i]->pos, cs);
			params->name = "Resource Station";
			params->rad = MAX(params->p->range * params->sol->rad * 20. / params->p->st.st.w, ((struct entity_private_static*)rstations[i]->vft)->hitradius);
			params->pointcolor[0] = rstations[i]->race == ppl->race ? 0 : 255;
			params->pointcolor[1] = rstations[i]->race != ppl->race && 0 <= rstations[i]->race ? 0 : 255;
			params->pointcolor[2] = 0;
			params->textcolor[0] = rstations[i]->race == ppl->race ? 0 : 255;
			params->textcolor[1] = rstations[i]->race != ppl->race && 0 <= rstations[i]->race ? 0 : 255;
			params->textcolor[2] = 0;
			if(drawSolarMapItem(params))
				params->p->targetr = rstations[i];
		}*/
		rrange = this->range * params->sol->csrad * 10. / this->w;
		collapse = cs->csrad < rrange;
		for(pt = cs->w->el; pt; pt = pt->next) if(strcmp(pt->classname(), "rstation") && (pt->race == ppl->race /*|| race_entity_visible(ppl->race, pt)*/)){
			avec3_t warpdst, pos1;
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
				params->rad = std::max(rrange, pt->hitradius());
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
	if(cs->toOrbitCS() && cs->toOrbitCS()->flags2 & OCS_SHOWORBIT){
		const OrbitCS *a = cs->toOrbitCS();
		int n, j;
		double (*cuts)[2], rad;
		const Astrobj *home = a->orbit_home;
		Vec3d spos, apos, apos0;
		Mat4d mat, qmat, rmat, lmat;
		Quatd q;
		double smia;
		double md;
		cuts = CircleCuts(64);
		spos = vwcs->tocs(home->pos, home->parent);
		params->apos0 = vwcs->tocs(cs->pos, cs->parent);
		apos = this->rot.trans(apos0);
		mat = vwcs->tocsim(home->parent);
		qmat = a->orbit_axis.tomat4();
		rad = a->orbit_rad;
		smia = rad * (a->eccentricity != 0. ? sqrt(1. - a->eccentricity * a->eccentricity) : 1.);
		qmat.scalein(rad, smia, rad);
		if(a->eccentricity != 0.)
			qmat.translatein(a->eccentricity, 0, 0);
		rmat = mat * qmat;
		rmat.vec3(3) += spos;
		lmat = params->viewmat * rmat;
		glColor4ub(255,191,255,255);
		glBegin(GL_LINE_LOOP);
		for(j = 0; j < 64; j++){
			avec3_t v, vr;
			v[0] = 0 + cuts[j][0];
			v[1] = 0 + cuts[j][1];
			v[2] = 0;
			mat4vp3(vr, lmat, v);
/*			VECADDIN(vr, spos);*/
			glVertex3dv(vr);
		}
		glEnd();
#if 1
		params->name = cs->fullname ? cs->fullname : cs->name;
		params->rad = cs->csrad;
/*		drawSolarMapItem(params);*/
#else
		md = (apos[0] - pointer[0]) * (apos[0] - pointer[0]) + (apos[1] - pointer[1]) * (apos[1] - pointer[1]);
		glPointSize(md < hitrad * hitrad ? 5 : 3);
		glColor4ub(191,255,191,255);
		glBegin(GL_POINTS);
		glVertex3dv(apos0);
		glEnd();
		if(md < hitrad * hitrad){
			avec3_t vofs, vofs0 = {0};
			vofs0[1] = *ofs;
			quatirot(vofs, rot, vofs0);
			glColor4ub(255,255,191,255);
			if(*ofs == 0.)
				VECCPY(pos, apos0);
			glRasterPos3d(pos[0] + vofs[0], pos[1] + vofs[1], pos[2] + vofs[2]);
			*ofs -= hitrad * 1.5;
			gldprintf(cs->fullname ? cs->fullname : cs->name);
		}
/*		glBegin(GL_LINES);
		glVertex3dv(pointer);
		glVertex3dv(apos);
		glEnd();*/
#endif
	}
	if((cs->flags & (CS_EXTENT | CS_ISOLATED)) == (CS_EXTENT | CS_ISOLATED) && 0 < cs->aorder.size()){
		Astrobj *a2;
		Vec3d spos, apos;
		double md;
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
	GLint vp[4], nvp[4];
	double dt = p->lastt ? gametime - p->lastt : 0.;
	int mi = MIN(this->w, this->h - 12) - 1;
	int ma = MAX(this->w, this->h - 12) - 1;
	double fx = (double)this->w / ma, fy = (double)(this->h - 12) / ma;
	double f;

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
		int i, ip[2], h = this->h - 12, iofs = 0;
		double range = sol->csrad * p->range;
		double hitrad = sol->csrad * .05 * p->range * (this->h - 12) / ma;
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
			p->org = sol->tocs(ppl->pos, ppl->cs);
			p->org *= 1. / sol->csrad;
		}
		if(p->sync){
			p->rot = ppl->rot;
		}
		params.viewmat = p->rot.tomat4();
		params.viewmat.translatein(-p->org[0] * sol->csrad, -p->org[1] * sol->csrad, -p->org[2] * sol->csrad);

		if(this){
			RECT rc;
			GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
			glGetIntegerv(GL_VIEWPORT, vp);
			glViewport(nvp[0] = this->x + 1, nvp[1] = rc.bottom - rc.top - (this->y + this->h) + 1, nvp[2] = this->w - 1, nvp[3] = this->h - 12 - 1);
		}

		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		glLoadIdentity();
	/*	glOrtho(-1., 1., -1., 1., -1., 1.);*/
		glOrtho(-fx, fx, -fy, fy, -1. / p->range, 1. / p->range);
		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glLoadIdentity();
/*		gldMultQuat(p->rot);
		glTranslated(-p->org[0] / p->range, -p->org[1] / p->range, -p->org[2] / p->range);*/
		gldScaled(1. / range);
		if(p->hold/* && p->pointer[0] - 10 <= mousex && mousex < p->pointer[0] + 100 && p->pointer[1] - 10 <= mousey && mousey < p->pointer[1] + 100*/){
			ip[0] = p->pointer[0];
			ip[1] = p->pointer[1];
		}
		else{
			ip[0] = mousex;
			ip[1] = mousey;
			p->hold = 0;
		}
		params.spointer[0] = (2 * range * ip[0] / this->w - range) * fx;
		params.spointer[1] = (-2 * range * ip[1] / (this->h - 12) + range) * fy;
		params.spointer[2] = 0.;
		params.org = p->rot.trans(p->org);
		params.spointer += org * sol->csrad;
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
		for(i = 0; i < ntplist; i++){
/*			double md;
			avec3_t spos, apos, apos0;*/
			params.apos0 = sol->tocs(tplist[i].pos, tplist[i].cs);
#if 1
			params.name = tplist[i].name;
			params.rad = 0.;
			if(drawSolarMapItem(&params))
				p->target = &tplist[i];
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
			Vec3d plpos0 = sol->tocs(ppl->pos, ppl->cs);
			plpos0 += p->org * -sol->csrad;
			plpos0 *= 1. / range * this->w;
			plpos = p->rot.trans(plpos0);
		}
		glPushMatrix();
		glLoadIdentity();
		glScaled(1. / this->w, 1. / this->w, 1. / this->w);
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
			apos0 *= 1. / range * this->w;
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
			int i, x, y;

			glColor4ub(127,127,127,255);
			glBegin(GL_LINES);
			x = this->x + this->w - scalespace;
			glVertex2d(x - scalewidth / 2, this->y + scalevspace + 12);
			glVertex2d(x - scalewidth / 2, this->y + this->h - scalevspace);
			for(i = 0; i <= scalerange; i++){
				y = this->y + scalevspace + 12 + (this->h - scalevspace * 2 - 12) * i / scalerange;
				glVertex2d(x - scalewidth, y);
				glVertex2d(x, y);
			}
			y = this->y + scalevspace + 12 + (this->h - scalevspace * 2 - 12) * (scalerange + log10(p->range)) / scalerange;
			glEnd();
			glColor4ub(127,255,127,255);
			glBegin(GL_QUADS);
			glVertex2d(x, y - 3);
			glVertex2d(x - scalewidth, y - 3);
			glVertex2d(x - scalewidth, y + 3);
			glVertex2d(x, y + 3);
			glEnd();
			glwpos2d(x - scalewidth / 2 - 8 / 2, this->y + 12 + scalevspace + scalewidth - 10);
			glwprintf("+");
			glwpos2d(x - scalewidth / 2 - 8 / 2, this->y + this->h - scalevspace - scalewidth + 20);
			glwprintf("-");
		}

		glColor4ub(255,255,255,255);
		glwpos2d(this->x + 2, this->y + this->h - 12);
		glwprintf("%s", sol->fullname ? sol->fullname : sol->name);
		glwpos2d(this->x + 2, this->y + this->h - 2);
		glwprintf("Range: %lg km", range);

		if(0 <= mousex && mousex < 48 && 0 <= mousey && mousey <= 12){
			int ind = mousex / 12;
			glColor4ub(191,31,31,127);
			glBegin(GL_QUADS);
			glVertex2d(this->x + ind * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12 + 12);
			glVertex2d(this->x + (ind) * 12, this->y + 12 + 12);
			glEnd();
			glColor4ub(191,255,255,255);
			glwpos2d(this->x + 48 + 2, this->y + (2) * 12 - 2);
			glwprintf(ind == 0 ? "Reset View" : ind == 1 ? "Defocus" : ind == 2 ? "Current Position" : "Synchronize Rotation");
		}
		if(p->focus || p->focusa){
			int ind = 1;
			glColor4ub(31,127,127,127);
			glBegin(GL_QUADS);
			glVertex2d(this->x + ind * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12 + 12);
			glVertex2d(this->x + (ind) * 12, this->y + 12 + 12);
			glEnd();
		}
		if(p->focusc){
			int ind = 2;
			glColor4ub(31,127,127,127);
			glBegin(GL_QUADS);
			glVertex2d(this->x + ind * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12 + 12);
			glVertex2d(this->x + (ind) * 12, this->y + 12 + 12);
			glEnd();
		}
		if(p->sync){
			int ind = 3;
			glColor4ub(31,127,127,127);
			glBegin(GL_QUADS);
			glVertex2d(this->x + ind * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12);
			glVertex2d(this->x + (ind + 1) * 12, this->y + 12 + 12);
			glVertex2d(this->x + (ind) * 12, this->y + 12 + 12);
			glEnd();
		}
		glColor4ub(191,255,255,255);
		glwpos2d(this->x + 1, this->y + (2) * 12);
		glwprintf("R");
		glwpos2d(this->x + 1 + 12, this->y + (2) * 12);
		glwprintf("F");
		glwpos2d(this->x + 1 + 24, this->y + (2) * 12);
		glwprintf("C");
		glwpos2d(this->x + 1 + 36, this->y + (2) * 12);
		glwprintf("S");
		glBegin(GL_LINES);
		glVertex2d(this->x, this->y + 12 + 12);
		glVertex2d(this->x + 48, this->y + 12 + 12);
		glVertex2d(this->x + 12, this->y + 12 + 12);
		glVertex2d(this->x + 12, this->y + 12);
		glVertex2d(this->x + 24, this->y + 12 + 12);
		glVertex2d(this->x + 24, this->y + 12);
		glVertex2d(this->x + 36, this->y + 12 + 12);
		glVertex2d(this->x + 36, this->y + 12);
		glVertex2d(this->x + 48, this->y + 12 + 12);
		glVertex2d(this->x + 48, this->y + 12);
		glEnd();
/*		printf("sm-all %lg\n", TimeMeasLap(&tm));*/
	}
}

void GLwindowSolarMap::drawInt(const CoordSys *cs, drawSolarMapItemParams &params){
	for(CoordSys::AOList::const_iterator i = cs->aorder.begin(); i != cs->aorder.end(); i++){
		const OrbitCS *a = (*i)->toOrbitCS();
		if(!a)
			continue;
		int n, j;
		double (*cuts)[2], rad;
		const Astrobj *home = a->orbit_home;
		Vec3d spos, apos, apos0;
		Mat4d mat, qmat, rmat, lmat;
		Quatd q;
		double smia;
		double md;
		if(sol != a->parent && !sol->is_ancestor_of(a->parent))
			continue;
		params.apos0 = sol->tocs(a->pos, a->parent);
		if(home){
			cuts = CircleCuts(64);
			spos = sol->tocs(home->pos, home->parent);
			mat = sol->tocsim(home->parent);
			qmat = a->orbit_axis.tomat4();
			rad = a->orbit_rad;
			smia = rad * (a->eccentricity != 0. ? sqrt(1. - a->eccentricity * a->eccentricity) : 1.);
			qmat.scalein(rad, smia, rad);
			if(a->eccentricity != 0.)
				qmat.translatein(a->eccentricity, 0, 0);
			rmat = mat * qmat;
			rmat.vec3(3) += spos;
			lmat = params.viewmat * rmat;
			glColor4ub(255,191,255,255);
			glBegin(GL_LINE_LOOP);
			for(j = 0; j < 64; j++){
				Vec3d v, vr;
				v[0] = 0 + cuts[j][0];
				v[1] = 0 + cuts[j][1];
				v[2] = 0;
				vr = lmat.vp3(v);
/*					VECADDIN(vr, spos);*/
				glVertex3dv(vr);
			}
			glEnd();
		}
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

int GLwindowSolarMap::mouse(int mbutton, int state, int mx, int my){
	extern Player *ppl;
	if(st::mouse(mbutton, state, mx, my))
		return 1;
	if(0);
#if 0
/*	if(my < 12)
		return 1;*/
	if(0 <= my && my <= 12 && 0 <= mx && mx < 48){
		int ind = mx / 12;
		if(state == GLUT_DOWN && mbutton == GLUT_LEFT_BUTTON) switch(ind){
			case 0: VECNULL(p->org); QUATIDENTITY(p->rot); return 1;
			case 2: p->focusc = !p->focusc; /* fall through */
			case 1: p->focus = NULL; p->focusa = NULL; return 1;
			case 3: p->sync = !p->sync; return 1;
		}
		return 1;
	}
	if(wnd->w - scalespace - scalewidth <= mx && mx <= wnd->w - scalespace && scalevspace <= my && my <= wnd->h - scalevspace - 12){
/*		y = wnd->y + scalespace + 12 + (wnd->h - scalespace * 2 - 12) * (scalerange + log10(p->range)) / scalerange;*/
		if((state == GLUT_KEEP_DOWN || state == GLUT_DOWN) && mbutton == GLUT_LEFT_BUTTON)
			p->dstrange = pow(10, (my - scalevspace) * scalerange / (wnd->h - scalevspace * 2 - 12) - scalerange);
		return 1;
	}
	if(state == GLUT_UP && (mbutton == GLUT_LEFT_BUTTON || mbutton == GLUT_RIGHT_BUTTON) && p->targete && p->hold != 2){
		if(p->targetc){
			entity_t *pt;
			ppl->selected = p->targetc->tl;
			for(pt = p->targetc->tl; pt; pt = pt->next){
				pt->selectnext = pt->next;
			}
		}
		else{
			ppl->selected = p->targete;
			p->targete->selectnext = NULL;
		}
		if(mbutton != GLUT_LEFT_BUTTON){
			entity_popup(p->targete, 1);
		}
		p->hold = 2;
		p->pointer[0] = p->pointer[1] = 0;
	}
	if(state == GLUT_UP && mbutton == GLUT_LEFT_BUTTON){
		if((p->target || p->targeta || p->targetr) && p->hold != 2){
#if defined _WIN32
			extern HWND hWndApp;
#endif
			int cmd_teleport(int argc, char *argv[]), cmd_warp(int argc, char *argv[]);
			char *argv[3];
			char buf[256];
			char *name = p->target ? p->target->name : p->targeta ? p->targeta->name : p->targetr->vft->classname(p->targetr);
			const char *typestring = p->target ? "Teleport" : "Astro";
#if 1
			char titles0[5][128], *titles[5];
			int keys[5];
			char cmds0[5][128], *cmds[5];
			int i = 0, j = 0;
			glwindow *glw;
			extern int s_mousex, s_mousey;

			sprintf(titles[j] = titles0[i], "Focus");
			keys[j] = 0;
			sprintf(cmds[j] = cmds0[i], "focus \"%s\"", name);
			i++; j++;
			if(p->target && p->target->flags & TELEPORT_TP){
				sprintf(titles[j] = titles0[i], "Teleport");
				keys[j] = 0;
				sprintf(cmds[j] = cmds0[i], "teleport \"%s\"", name);
				i++; j++;
			}

			if(p->target && p->target->flags & TELEPORT_WARP || p->targetr){
				sprintf(titles[j] = titles0[i], "Warp");
				keys[j] = 0;
				if(p->targetr)
					sprintf(cmds[j] = cmds0[i], "warp \"%s\" %lg %lg %lg", p->targetr->w->cs->name, p->targetr->pos[0], p->targetr->pos[1] + 1., p->targetr->pos[2]);
				else
					sprintf(cmds[j] = cmds0[i], "warp \"%s\"", name);
				i++; j++;
			}

			sprintf(titles[j] = titles0[i], "Information");
			keys[j] = 0;
			sprintf(cmds[j] = cmds0[i], "info %s \"%s\"", typestring, name);
			i++; j++;

			glw = glwPopupMenu(j, titles, keys, cmds, 0);
			glw->x = s_mousex;
			glw->y = s_mousey;

			p->hold = 2;
			p->pointer[0] = p->pointer[1] = 0;
#else
			static HMENU hm = NULL;
			MENUITEMINFO mii;
			POINT cursorpos;
			BOOL (WINAPI *MenuItem)(
  HMENU hMenu,          // handle to menu
  UINT uItem,           // identifier or position
  BOOL fByPosition,     // meaning of uItem
  LPMENUITEMINFO lpmii  // menu item information
  ) = hm ? SetMenuItemInfo : InsertMenuItem;
/*			argv[0] = mbutton == GLUT_LEFT_BUTTON ? "teleport" : "warp";
			argv[1] = p->target->name;
			argv[2] = NULL;
			(mbutton == GLUT_LEFT_BUTTON ? cmd_teleport : cmd_warp)(2, argv);*/
			if(!hm)
				hMapPopupMenu = hm = CreatePopupMenu();
			mii.cbSize = sizeof mii;
			mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
			mii.fType = MIIM_STRING;
			mii.dwTypeData = buf;

			sprintf(buf, "focus \"%s\"", name);
			mii.wID = 4003;
			mii.fState = MFS_ENABLED;
			MenuItem(hm, 0, TRUE, &mii);

			sprintf(buf, "teleport \"%s\"", name);
			mii.wID = 4000;
			mii.fState = p->target && p->target->flags & TELEPORT_TP ? MFS_ENABLED : MFS_DISABLED;
			MenuItem(hm, 1, TRUE, &mii);

			if(p->targetr)
				sprintf(buf, "warp \"%s\" %lg %lg %lg", p->targetr->w->cs->name, p->targetr->pos[0], p->targetr->pos[1] + 1., p->targetr->pos[2]);
			else
				sprintf(buf, "warp \"%s\"", name);
			mii.wID = 4001;
			mii.fState = p->target && p->target->flags & TELEPORT_WARP || p->targetr ? MFS_ENABLED : MFS_DISABLED;
			MenuItem(hm, 2, TRUE, &mii);

			sprintf(buf, "info %s \"%s\"", typestring, name);
			mii.wID = 4002;
			mii.fState = MFS_ENABLED;
			MenuItem(hm, 3, TRUE, &mii);

			GetCursorPos(&cursorpos);
			TrackPopupMenu(hm, TPM_LEFTBUTTON, cursorpos.x, cursorpos.y, 0, hWndApp, NULL);
			p->hold = 2;
			p->pointer[0] = p->pointer[1] = 0;
#endif
		}
	}
#endif
	if(state == GLUT_DOWN){
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
			dr0[0] = 2. * range * -(mx - morg[0]) / this->w;
			dr0[1] = 2. * range * (my - morg[1]) / this->w;
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
		case '[': w *= 2, h = w + 12; break;
		case ']': w /= 2, h = w + 12; break;
	}
	return 1;
}

void GLwindowSolarMap::anim(double dt){
	double f = 1. - exp(-dt / .10);
	range += (dstrange - range) * f;
}

GLwindow *GLwindowSolarMap::showWindow(Player *ppl){
	glwindow *ret;
	glwindow **ppwnd;
	int i;
	static const char *windowtitle = "Solarsystem browser";
	ppwnd = findpp(&glwlist, &namecmp);
	if(!ppwnd){
		glwActivate(ppwnd = glwAppend(new GLwindowSolarMap(windowtitle, ppl)));
	}
	else
		glwActivate(ppwnd);
/*	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}*/
/*
	ret = &p->st;
	glwsizeable_init(&p->st);*/
/*	ret->flags |= GLW_SIZEPROP;*/
	return *ppwnd;
}


int cmd_togglesolarmap(int argc, char *argv[], void *pv){
	GLwindowSolarMap::showWindow((Player*)pv);
	return 0;
}







































#if 0
struct glwindowinfo{
	struct glwindowsizeable st;
	int type;
	struct teleport *tp;
	Astrobj *a;
};

static void glwinfo_draw(glwindow *wnd, int mx, int my, double t){
	struct glwindowinfo *p = (struct glwindowinfo *)wnd;
	int iy = 0;
	glColor4ub(255,213,213,255);
	if(p->type == 1){
		if(!p->a)
			return;
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Astronomical Object");
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Name: %s", p->a->name);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("CoordSys: %s", p->a->cs->name);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Position: %lg,%lg,%lg", p->a->pos[0], p->a->pos[1], p->a->pos[2]);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Radius: %lg km", p->a->rad);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Mass: %lg kg", p->a->mass);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Flags: ");
		if(p->a->flags & AO_PLANET)
			glwprintf("Planet ");
		if(p->a->flags & AO_GRAVITY)
			glwprintf("Gravity ");
		if(p->a->flags & AO_SPHEREHIT)
			glwprintf("SphereHit ");
		if(p->a->flags & AO_PLANET && p->a->orbit_home){
			glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
			glwprintf("Orbits %s", p->a->orbit_home->name);
			glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
			glwprintf("Semi-major axis: %lg km", p->a->orbit_radius);
			glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
			glwprintf("Eccentricity: %lg", p->a->eccentricity);
		}
	}
	else if(p->type == 3){
		if(!p->tp)
			return;
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Teleport Site");
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Name: %s", p->tp->name);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("CoordSys: %s", p->tp->cs->name);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Position: %lg,%lg,%lg", p->tp->pos[0], p->tp->pos[1], p->tp->pos[2]);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Flags: ");
		if(p->tp->flags & TELEPORT_TP)
			glwprintf("Teleportable ");
		if(p->tp->flags & TELEPORT_WARP)
			glwprintf("Warpable ");
	}
}

static int glwinfo_mouse(glwindow *wnd, int mbutton, int state, int mx, int my){
	if(glwsizeable_mouse(wnd, mbutton, state, mx, my))
		return 1;
	return 0;
}

static int glwinfo_key(glwindow *wnd, int key){
	return 0;
}

glwindow *glwInfo(int type, const char *name){
	glwindow *ret;
	struct glwindowinfo *p;
	int i;
	p = malloc(sizeof *p + sizeof " Property" + strlen(name) + 1);
	sprintf((char*)&p[1], "%s Property", name);
	ret = &p->st.st;
	ret->x = 75;
	ret->y = 75;
	ret->w = 260;
	ret->h = 2 * 12 + 10 * 12;
	ret->title = (char*)&p[1];
	ret->next = glwlist;
	glwlist = ret;
	glwActivate(&glwlist);
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE;
	ret->modal = NULL;
	ret->draw = glwinfo_draw;
	ret->mouse = glwinfo_mouse;
	ret->key = glwinfo_key;
	ret->destruct = NULL;
	glwsizeable_init(&p->st);
	p->type = type;
	p->tp = NULL;
	p->a = NULL;
	if(type == 0 || type == 3){
		for(i = 0; i < ntplist; i++) if(!strcmp(tplist[i].name, name)){
			p->tp = &tplist[i];
			if(type == 0)
				p->type = 3;
			break;
		}
	}
	if(type == 0 || type == 1){
		extern Astrobj **astrobjs;
		extern int nastrobjs;
		for(i = 0; i < nastrobjs; i++) if(!strcmp(astrobjs[i]->name, name)){
			p->a = astrobjs[i];
			if(type == 0)
				p->type = 1;
			break;
		}
	}
	if(p->tp == NULL && p->a == NULL){
		ret->flags |= GLW_TODELETE;
		ret = -1;
	}
	return ret;
}

int cmd_info(int argc, const char *argv[]){
	glwindow *ret;
	if(argc < 2){
		CmdPrint("usage: info target");
		return 0;
	}
	ret = glwInfo(argc < 3 ? 0 : !strcmp(argv[1], "Astro") ? 1 : !strcmp(argv[1], "Coordsys") ? 2 : !strcmp(argv[1], "Teleport") ? 3 : 0, argv[argc < 3 ? 1 : 2]);
/*	if(glwcmdmenu == glwfocus)
		glwfocus = ret;
	glwcmdmenu = ret;*/
	return 0;
}

int cmd_focus(int argc, const char *argv[]){
	struct glwindowsolmap *sm = glwfocus ? glwfocus : glwlist;
	int i;
	extern Astrobj **astrobjs;
	extern int nastrobjs;

	for(sm = glwlist; sm; sm = sm->st.st.next) if(sm->st.st.draw == solarmap_draw)
		break;
	if(!sm)
		return 1;

	/* Always reset */
	if(sm->focus && !strcmp(sm->focus->name, argv[1]) ||
		sm->focusa && !strcmp(sm->focusa->name, argv[1])||
		argc < 2)
	{
		sm->focus = NULL;
		sm->focusa = NULL;
		sm->focusc = 0;
		return 0;
	}

	for(i = 0; i < nastrobjs; i++) if(!strcmp(astrobjs[i]->name, argv[1])){
		sm->focusa = astrobjs[i];
		sm->focus = NULL;
		sm->focusc = 0;
		return 0;
	}
	for(i = 0; i < ntplist; i++) if(!strcmp(tplist[i].name, argv[1])){
		sm->focus = &tplist[i];
		sm->focusa = NULL;
		sm->focusc = 0;
		return 0;
	}
	return 0;
}
#endif