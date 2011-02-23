#include "WarMap.h"
/*#include "drawmap.h"
#include "glwindow.h"
#include "antiglut.h"
#include "player.h"
#include "entity.h"
#include "war.h"*/
#include <cpplib/vec4.h>
#include <cpplib/quat.h>
#include <cpplib/mat4.h>
extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
}
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

WarMap::WarMap() : alt(NULL), nalt(0){}

WarMap::~WarMap(){}

Vec3d WarMap::normal(double x, double y){
	return Vec3d(0,0,1);
}

double WarMap::height(double x, double y, Vec3d *normal){
	wartile_t a;
	double xf, yf;
	int sx, sy;
	int size;
	double width;
	double orgx, orgy;
	double cell;
	double ret;
	if(!this){
		if(normal)
			*normal = vec3_001;
		return 0.;
	}
	this->size(&sx, &sy);
	size = MAX(sx, sy);
	width = this->width();
	cell = width / size;
	orgx = -width / 2.;
	orgy = -width / 2.;
	x -= orgx;
	y -= orgy;
	if(x < 0. || (sx-1) * cell < x || y < 0. || (sy-1) * cell < y){
		if(normal)
			*normal = vec3_001;
		return 0.;
	}
#if 1

	/* normalize the coordinates in the cell. */
	xf = fmod(x, cell);
	yf = fmod(y, cell);

	if(yf < xf){
		Vec3d p0(0, 0, 0), p1(cell, 0., 0), p2(cell, cell, 0);
		getat(&a, (int)(x / cell), (int)(y / cell));
		p0[2] = a.height;
		getat(&a, (int)(x / cell) + 1, (int)(y / cell));
		p1[2] = a.height;
		getat(&a, (int)(x / cell) + 1, (int)(y / cell) + 1);
		p2[2] = a.height;
		Vec3d d01 = p1 - p0;
		Vec3d d02 = p2 - p0;
		Vec3d n = d01.vp(d02);
		if(normal)
			*normal = n;
		ret = -(n[0] * xf + n[1] * yf) / n[2] + p0[2];
	}
	else{
		Vec3d p0(0, 0, 0), p1(0., cell, 0), p2(cell, cell, 0);
		getat(&a, (int)(x / cell), (int)(y / cell));
		p0[2] = a.height;
		getat(&a, (int)(x / cell), (int)(y / cell) + 1);
		p1[2] = a.height;
		getat(&a, (int)(x / cell) + 1, (int)(y / cell) + 1);
		p2[2] = a.height;
		Vec3d d01 = p1 - p0;
		Vec3d d02 = p2 - p0;
		Vec3d n = d02.vp(d01);
		if(normal)
			*normal = n;
		ret = -(n[0] * xf + n[1] * yf) / n[2] + p0[2];
	}

	return ret;
#else
	wm->vft->getat(wm, &a, (int)(x / cell), (int)(y / cell));
	wm->vft->getat(wm, &b, (int)(x / cell) + 1, (int)(y / cell));
	wm->vft->getat(wm, &c, (int)(x / cell), (int)(y / cell) + 1);
	wm->vft->getat(wm, &d, (int)(x / cell) + 1, (int)(y / cell) + 1);
	xf = (x - (int)(x / cell) * cell) / cell;
	ab = (1. - xf) * a.height + xf * b.height;
	cd = (1. - xf) * c.height + xf * d.height;
	yf = (y - (int)(y / cell) * cell) / cell;
	return (1. - yf) * ab + yf * cd;
#endif
}

double WarMap::heightr(double x, double y, Vec3d *normal){
	WarMap *wm = this;
	double ret = height(x, y, normal);
#if 0
	{
		int j, i, l, n = 64;
		avec3_t cp0[16];
		Vec3d cp1[4];
		double *va = cp1[0], *vb = cp1[1], *va1 = cp1[2], *vb1 = cp1[3];
		Vec3d v1, v2, vn1;
		for(j = 0; j < (numof(cp0) - 4) / 2; j++){
			Vec3d cp[4];
			Vec3d p4, p5, p6, p7, p8, p9;
			double xmax, xmin, ymax, ymin;
			cp[0] = cp0[j*2];
			cp[1] = cp0[j*2+1] * .5;
			cp[1] += cp[0];
			cp[3] = cp0[j*2+2];
			cp[2] = cp0[j*2+3] * -.5;
			cp[2] += cp[3];
			xmin = xmax = cp[0][0], ymin = ymax = cp[0][2];
			for(i = 0; i < 4; i++){
				if(xmax < cp[i][0])
					xmax = cp[i][0];
				if(cp[i][0] < xmin)
					xmin = cp[i][0];
				if(ymax < cp[i][2])
					ymax = cp[i][2];
				if(cp[i][2] < ymin)
					ymin = cp[i][2];
			}
			if(x < xmin || xmax < x || y < ymin || ymax < y)
				continue;
			for(i = 0; i < n; i++){
				int k;
				VECSCALE(p4, cp[0], (double)(n - i) / n);
				VECSADD(p4, cp[1], (double)(i) / n);
				VECSCALE(p5, cp[1], (double)(n - i) / n);
				VECSADD(p5, cp[2], (double)(i) / n);
				VECSCALE(p6, cp[2], (double)(n - i) / n);
				VECSADD(p6, cp[3], (double)(i) / n);
				VECSCALE(p7, p4, (double)(n - i) / n);
				VECSADD(p7, p5, (double)(i) / n);
				VECSCALE(p8, p5, (double)(n - i) / n);
				VECSADD(p8, p6, (double)(i) / n);
				VECSCALE(p9, p7, (double)(n - i) / n);
				VECSADD(p9, p8, (double)(i) / n);
				if(1 < i || j){
					VECSUB(p4, p9, v1);
					VECVP(p7, p4, avec3_010);
					VECNORMIN(p7);
					VECSUB(p5, v1, v2);
					VECVP(p8, p5, avec3_010);
					VECNORMIN(p8);
					VECADD(p6, p7, p8);
					p7[0] = -VECSP(p6, avec3_010);
					VECSADD(p6, avec3_010, p7[0]);
					VECNORMIN(p6);
//					VECVP(va, p4, avec3_010);
					VECVP(p5, p6, p4);
					VECNORMIN(p5);
					VECSCALE(va, p6, .005);
					VECSCALE(vb, va, -1);
					VECADDIN(va, v1);
					VECADDIN(vb, v1);
					VECCPY(va1, va);
					VECCPY(vb1, vb);
					VECCPY(vn1, p5);
				}
				if(i || j){
					VECCPY(v2, v1);
				}
				else{
					VECCPY(va1, p9);
					VECCPY(vb1, p9);
					VECCPY(vn1, avec3_010);
				}
				VECCPY(v1, p9);
/*				if((p9[0] - x) * (p9[0] - x) + (p9[2] - y) * (p9[2] - y) < (xmax - xmin + ymax - ymin) / n)
					break;*/
				if(!(i || j))
					continue;
				xmin = xmax = cp1[0][0], ymin = ymax = cp1[0][2];
				for(k = 0; k < 4; k++){
					if(xmax < cp1[k][0])
						xmax = cp1[k][0];
					if(cp1[k][0] < xmin)
						xmin = cp1[k][0];
					if(ymax < cp1[k][2])
						ymax = cp1[k][2];
					if(cp1[k][2] < ymin)
						ymin = cp1[k][2];
				}
				if(x < xmin || xmax < x || y < ymin || ymax < y)
					continue;
				break;
			}
			if(i == n)
				continue;
			return ret + .005;
		}
	}
#endif
	return ret;
}


int WarMap::linehit(const Vec3d &src, const Vec3d &dir, double t, Vec3d &ret){
	return 0;
}


#if 0
typedef struct player player_t;

struct glwindowmap{
	struct glwindowsizeable st;
	double org[2];
	double lastt;
	double range;
	int morg[2];
	warmap_t *map;
	warf_t *w;
	player_t *pl;
};


static void drawminimap(struct glwindowmap *p, double scale, const avec3_t pos){
	double rangle, angle;
	warmap_t *wm = p->map;
	{
		amat4_t rot;
		avec3_t pyr;
		getrot(rot);
		imat2pyr(rot, pyr);
		rangle = pyr[1];
		if(r_maprot)
			angle = rangle;
		else
			angle = 0.;
	}
/*	glColor4ubv(hud_color_options[hud_color]);*/
	glColor4ub(255,255,255,255);
	if(DrawMiniMap(wm, pos, scale, angle)){

		/*if(!pl.chase)*/{
			entity_t *pt;
			amat4_t mat, mat2;
			glPushAttrib(GL_POINT_BIT | GL_CURRENT_BIT);
			glPointSize(2);
			glDisable(GL_POINT_SMOOTH);
			glPushMatrix();
			MAT4IDENTITY(mat);
			mat4rotz(mat2, mat, angle);
			MAT4SCALE(mat2, 1. / scale, 1. / scale, 1.);
			mat4translate(mat2, -pos[0], pos[2], 0.);
			glBegin(GL_POINTS);
			for(pt = p->w->tl; pt; pt = pt->next) if(pt->active){
				avec3_t v, v0;
				v0[0] = pt->pos[0], v0[1] = -pt->pos[2], v0[2] = 0.;
				mat4vp3(v, mat2, v0);
				if(-1. <= v[0] && v[0] <= 1. && -1. <= v[1] && v[1] <= 1.){
					if(pt->race < 0)
						glColor4ub(127,127,127,255);
					else
						glColor4ub(pt->race & 1 ? 255 : 0, pt->race & 2 ? 255 : 0, pt->race & 4 ? 255 : 0, 255);
					glVertex3dv(v);
				}
			}
			glEnd();
			glColor4ub(255,255,255,255);
			{
				avec3_t v, v0;
				double (*cuts)[2];
				int i;
				v0[0] = p->pl->pos[0], v0[1] = -p->pl->pos[2], v0[2] = 0.;
				mat4vp3(v, mat2, v0);
				gldTranslate3dv(v);
				if(!r_maprot)
					glRotated(-rangle * deg_per_rad, 0, 0, 1);
				glBegin(GL_LINE_LOOP);
				glVertex2d(0, .1);
				glVertex2d(.05, -.1);
				glVertex2d(.0, -.0);
				glVertex2d(-.05, -.1);
				glEnd();
				cuts = CircleCuts(32);
				glBegin(GL_LINE_LOOP);
				for(i = 0; i < 32; i++){
					glVertex2d(cuts[i][0], cuts[i][1]);
				}
				glEnd();
			}
			glPopMatrix();
			glPopAttrib();
		}
	}
}


static int MiniMapShow(struct glwindow *wnd, int mx, int my, double gametime){
	struct glwindowmap *p = (struct glwindowmap *)wnd;
	double dt = p->lastt ? gametime - p->lastt : 0.;
	amat4_t proj;
	avec3_t dst, pos;
	GLint vp[4];
	double f;
	int ma = MAX(wnd->w, wnd->h - 12) - 1;
	double fx = (double)wnd->w / ma, fy = (double)(wnd->h - 12) / ma;
	f = 1. - exp(-dt / .10);

	if(wnd){
		RECT rc;
		int mi = MIN(wnd->w, wnd->h - 12) - 1;
		GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
		glGetIntegerv(GL_VIEWPORT, vp);
		glViewport(wnd->x + 1, rc.bottom - rc.top - (wnd->y + wnd->h) + 1, wnd->w - 1, wnd->h - 12 - 1);
	}

	p->lastt = gametime;
	p->range += (g_maprange - p->range) * f;

	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, &proj);
	glLoadIdentity();
	glOrtho(-fx, fx, -fy, fy, -1., 1.);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadIdentity();
	pos[0] = p->pl->pos[0] + p->org[0];
	pos[1] = p->pl->pos[1];
	pos[2] = p->pl->pos[2] + p->org[1];
	drawminimap(p, p->range, pos);
	glRasterPos2d(-fx + .01, -fy + .01);
	gldprintf("range %lg km", g_maprange);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(proj);
	glMatrixMode(GL_MODELVIEW);

	if(wnd){
		glViewport(vp[0], vp[1], vp[2], vp[3]);
	}

	return 0;
}

static int MiniMapMouse(struct glwindow *wnd, int mbutton, int state, int mx, int my){
	struct glwindowmap *p = (struct glwindowmap *)wnd;
	if(glwsizeable_mouse(wnd, mbutton, state, mx, my))
		return 1;
	if(my < 12)
		return 0;
	if(state == GLUT_DOWN){
		p->morg[0] = mx;
		p->morg[1] = my;
	}
	else if(state == GLUT_UP || state == GLUT_KEEP_DOWN){
		if(r_maprot){
			amat4_t rot;
			avec3_t pyr;
			double angle;
			double dx, dy;
			getrot(rot);
			imat2pyr(rot, pyr);
			angle = pyr[1];
			dx = 2. * g_maprange * -(mx - p->morg[0]) / wnd->w;
			dy = 2. * g_maprange * -(my - p->morg[1]) / wnd->w;
			p->org[0] += dx * cos(angle) + dy * -sin(angle);
			p->org[1] += dx * sin(angle) + dy * cos(angle);
		}
		else{
			p->org[0] += 2. * g_maprange * -(mx - p->morg[0]) / wnd->w;
			p->org[1] += 2. * g_maprange * -(my - p->morg[1]) / wnd->w;
		}
		p->morg[0] = mx;
		p->morg[1] = my;
	}
	else if(state == GLUT_WHEEL_DOWN)
		g_maprange /= 2.;
	else if(state == GLUT_WHEEL_UP)
		g_maprange *= 2.;
	return 1;
}

static int MiniMapKey(struct glwindow *wnd, int key){
	struct glwindowmap *p = (struct glwindowmap *)wnd;
	switch(key){
		case 'r': p->org[0] = p->org[1] = 0.; break;
		case '+': g_maprange /= 2.; break;
		case '-': g_maprange *= 2.; break;
		case '[': wnd->w *= 2, wnd->h = wnd->w + 12; break;
		case ']': wnd->w /= 2, wnd->h = wnd->w + 12; break;
	}
	return 1;
}

int cmd_toggleminimap(int argc, char *argv[]){
	extern warmap_t *g_map;
	extern warf_t warf;
	extern player_t *ppl;
	glwindow *ret;
	glwindow **ppwnd;
	struct glwindowmap *p;
	int i;
	static const char *windowtitle = "Field Map";
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}
	p = malloc(sizeof *p);
	ret = &p->st;
	ret->x = 50;
	ret->y = 50;
	ret->w = 250;
	ret->h = 262;
	ret->title = windowtitle;
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE;
	ret->draw = MiniMapShow;
	ret->mouse = MiniMapMouse;
	ret->key = MiniMapKey;
	ret->destruct = NULL;
	ret->modal = 0;
	glwsizeable_init(&p->st);
	p->morg[0] = p->morg[1] = 0;
	p->org[0] = p->org[1] = 0.;
	p->range = g_maprange;
	p->lastt = 0.;
	p->map = g_map;
	p->w = &warf;
	p->pl = ppl;
	glwActivate(glwAppend(ret));
	return 0;
}
#endif

#if 1
#elif 1
static void drawminimap(warmap_t *wm, double scale, const avec3_t pos){
	double rangle, angle;
	{
		amat4_t rot;
		avec3_t pyr;
		getrot(rot);
		imat2pyr(rot, pyr);
		rangle = pyr[1];
		if(r_maprot)
			angle = rangle;
		else
			angle = 0.;
	}
/*	glColor4ubv(hud_color_options[hud_color]);*/
	glColor4ub(255,255,255,255);
	if(DrawMiniMap(wm, pos, scale, angle)){

		/*if(!pl.chase)*/{
			entity_t *pt;
			amat4_t mat, mat2;
			glPushAttrib(GL_POINT_BIT | GL_CURRENT_BIT);
			glPointSize(2);
			glDisable(GL_POINT_SMOOTH);
			glPushMatrix();
			MAT4IDENTITY(mat);
			mat4rotz(mat2, mat, angle);
			MAT4SCALE(mat2, 1. / scale, 1. / scale, 1.);
			mat4translate(mat2, -pos[0], pos[2], 0.);
			glBegin(GL_POINTS);
			for(pt = warf.tl; pt; pt = pt->next) if(pt->active){
				avec3_t v, v0;
				v0[0] = pt->pos[0], v0[1] = -pt->pos[2], v0[2] = 0.;
				mat4vp3(v, mat2, v0);
				if(-1. <= v[0] && v[0] <= 1. && -1. <= v[1] && v[1] <= 1.){
					if(pt->race < 0)
						glColor4ub(127,127,127,255);
					else
						glColor4ub(pt->race & 1 ? 255 : 0, pt->race & 2 ? 255 : 0, pt->race & 4 ? 255 : 0, 255);
					glVertex3dv(v);
				}
			}
			glEnd();
			glColor4ub(255,255,255,255);
			{
				avec3_t v, v0;
				double (*cuts)[2];
				int i;
				v0[0] = pl.pos[0], v0[1] = -pl.pos[2], v0[2] = 0.;
				mat4vp3(v, mat2, v0);
				gldTranslate3dv(v);
				if(!r_maprot)
					glRotated(-rangle * deg_per_rad, 0, 0, 1);
				glBegin(GL_LINE_LOOP);
				glVertex2d(0, .1);
				glVertex2d(.05, -.1);
				glVertex2d(.0, -.0);
				glVertex2d(-.05, -.1);
				glEnd();
				cuts = CircleCuts(32);
				glBegin(GL_LINE_LOOP);
				for(i = 0; i < 32; i++){
					glVertex2d(cuts[i][0], cuts[i][1]);
				}
				glEnd();
			}
			glPopMatrix();
			glPopAttrib();
		}
	}
}


struct glwindowmap{
	struct glwindowsizeable st;
	double org[2];
	double lastt;
	double range;
	int morg[2];
};

static int MiniMapShow(struct glwindow *wnd, int mx, int my, double gametime){
	struct glwindowmap *p = (struct glwindowmap *)wnd;
	double dt = p->lastt ? gametime - p->lastt : 0.;
	amat4_t proj;
	avec3_t dst, pos;
	GLint vp[4];
	double f;
	int ma = MAX(wnd->w, wnd->h - 12) - 1;
	double fx = (double)wnd->w / ma, fy = (double)(wnd->h - 12) / ma;
	f = 1. - exp(-dt / .10);

	if(wnd){
		RECT rc;
		int mi = MIN(wnd->w, wnd->h - 12) - 1;
		GetClientRect(WindowFromDC(wglGetCurrentDC()), &rc);
		glGetIntegerv(GL_VIEWPORT, vp);
		glViewport(wnd->x + 1, rc.bottom - rc.top - (wnd->y + wnd->h) + 1, wnd->w - 1, wnd->h - 12 - 1);
	}

	p->lastt = gametime;
	p->range += (g_maprange - p->range) * f;

	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, &proj);
	glLoadIdentity();
	glOrtho(-fx, fx, -fy, fy, -1., 1.);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadIdentity();
	pos[0] = pl.pos[0] + p->org[0];
	pos[1] = pl.pos[1];
	pos[2] = pl.pos[2] + p->org[1];
	drawminimap(g_map, p->range, pos);
	glRasterPos2d(-fx + .01, -fy + .01);
	gldprintf("range %lg km", g_maprange);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(proj);
	glMatrixMode(GL_MODELVIEW);

	if(wnd){
		glViewport(vp[0], vp[1], vp[2], vp[3]);
	}

	return 0;
}

static int MiniMapMouse(struct glwindow *wnd, int mbutton, int state, int mx, int my){
	struct glwindowmap *p = (struct glwindowmap *)wnd;
	if(glwsizeable_mouse(wnd, mbutton, state, mx, my))
		return 1;
	if(my < 12)
		return 0;
	if(state == GLUT_DOWN){
		p->morg[0] = mx;
		p->morg[1] = my;
	}
	else if(state == GLUT_UP || state == GLUT_KEEP_DOWN){
		if(r_maprot){
			amat4_t rot;
			avec3_t pyr;
			double angle;
			double dx, dy;
			getrot(rot);
			imat2pyr(rot, pyr);
			angle = pyr[1];
			dx = 2. * g_maprange * -(mx - p->morg[0]) / wnd->w;
			dy = 2. * g_maprange * -(my - p->morg[1]) / wnd->w;
			p->org[0] += dx * cos(angle) + dy * -sin(angle);
			p->org[1] += dx * sin(angle) + dy * cos(angle);
		}
		else{
			p->org[0] += 2. * g_maprange * -(mx - p->morg[0]) / wnd->w;
			p->org[1] += 2. * g_maprange * -(my - p->morg[1]) / wnd->w;
		}
		p->morg[0] = mx;
		p->morg[1] = my;
	}
	else if(state == GLUT_WHEEL_DOWN)
		g_maprange /= 2.;
	else if(state == GLUT_WHEEL_UP)
		g_maprange *= 2.;
	return 1;
}

static int MiniMapKey(struct glwindow *wnd, int key){
	struct glwindowmap *p = (struct glwindowmap *)wnd;
	switch(key){
		case 'r': p->org[0] = p->org[1] = 0.; break;
		case '+': g_maprange /= 2.; break;
		case '-': g_maprange *= 2.; break;
		case '[': wnd->w *= 2, wnd->h = wnd->w + 12; break;
		case ']': wnd->w /= 2, wnd->h = wnd->w + 12; break;
	}
	return 1;
}

int cmd_toggleminimap(int argc, char *argv[]){
	glwindow *ret;
	glwindow **ppwnd;
	struct glwindowmap *p;
	int i;
	static const char *windowtitle = "Field Map";
	for(ppwnd = &glwlist; *ppwnd; ppwnd = &(*ppwnd)->next) if((*ppwnd)->title == windowtitle){
		glwActivate(ppwnd);
		return 0;
	}
	p = malloc(sizeof *p);
	ret = &p->st;
	ret->x = 50;
	ret->y = 50;
	ret->w = 250;
	ret->h = 262;
	ret->title = windowtitle;
	ret->flags = GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE;
	ret->modal = NULL;
	ret->draw = MiniMapShow;
	ret->mouse = MiniMapMouse;
	ret->key = MiniMapKey;
	ret->destruct = NULL;
	glwsizeable_init(&p->st);
	p->morg[0] = p->morg[1] = 0;
	p->org[0] = p->org[1] = 0.;
	p->range = g_maprange;
	p->lastt = 0.;
	glwActivate(glwAppend(ret));
	return 0;
}

#else
static void drawminimap(){
	static GLuint tex = 0;
	if(pl.cs != &earthsurface)
		return;
	if(!tex && g_map){
		int xs, ys;
		int mx, my;
		int x, y;
		double *dbuf, mi = 1000., ma = 0.;
		GLubyte *bbuf;
		g_map->vft->size(g_map, &xs, &ys);
		mx = MIN(xs, 512);
		my = MIN(ys, 512);
		dbuf = (double*)malloc(mx * my * sizeof(double));
		for(y = 0; y < my; y++) for(x = 0; x < mx; x++){
			wartile_t wt;
			double h;
			g_map->vft->getat(g_map, &wt, x * xs / mx, y * ys / my);
			dbuf[x + y * mx] = h = MAX(0, wt.height);
			if(h < mi)
				mi = h;
			if(ma < h)
				ma = h;
		}
		bbuf = (GLubyte*)malloc(mx * my * sizeof(GLubyte));
		for(y = 0; y < my; y++) for(x = 0; x < mx; x++){
			bbuf[x + y * mx] = (GLubyte)(255 * (dbuf[x + y * my] - mi) / (ma - mi));
		}

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mx, my, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, bbuf);

/*		glNewList(tex = glGenLists(1), GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);*/
		glEndList();
		free(dbuf);
		free(bbuf);
	}
	if(tex && g_map){
		double width, angle, cell, x0, y0, yaw, tc[4][2];
		width = g_map->vft->width(g_map);
		cell = g_maprange / width /* g_far / width*/;
		x0 = (pl.pos[0] + width / 2.) / width;
		y0 = (pl.pos[2] + width / 2.) / width;
/*		x = x0 - cell;
		y = y0 - cell;
		x1 = x + 2 * cell;
		y1 = y + 2 * cell;*/
		if(warf.maprot){
			int i;
			cell *= sqrt(2.);
			{
				amat4_t rot;
				avec3_t pyr;
				getrot(rot);
				imat2pyr(rot, pyr);
				angle = pyr[1];
			}
/*			if(pl.chase && pl.chase->active){
				entity_t *pt = pl.chase;
				if(pl.cs->flags & CS_PYR)
					angle = pt->pyr[1] + pt->turrety;
				else{
					amat4_t mat, mat2, mat3;
					avec3_t pyr;
					pt->vft->getrot(pt, &warf, mat);
					quat2mat(mat2, pl.rot);
					MAT4MP(mat3, mat2, mat);
					quat2pyr(mat3, pyr);
					angle = pyr[1];
				}
			}
			else{
				if(pl.cs->flags & CS_PYR)
					angle = pl.pyr[1];
				else{
					avec3_t pyr;
					quat2pyr(pl.rot, pyr);
					angle = pyr[1];
				}
			}*/
			tc[0][0] = cell * cos(angle + M_PI / 4.);
			tc[2][0] = -tc[0][0];
			tc[0][1] = cell * sin(angle + M_PI / 4.);
			tc[2][1] = -tc[0][1];
			tc[1][0] = cell * cos(angle + M_PI * 3 / 4.);
			tc[3][0] = -tc[1][0];
			tc[1][1] = cell * sin(angle + M_PI * 3 / 4.);
			tc[3][1] = -tc[1][1];
			for(i = 0; i < 4; i++){
				tc[i][0] += x0;
				tc[i][1] += y0;
			}
		}
		else{
			angle = 0.;
			tc[0][0] = tc[3][0] = x0 + cell;
			tc[1][0] = tc[2][0] = x0 - cell;
			tc[0][1] = tc[1][1] = y0 + cell;
			tc[2][1] = tc[3][1] = y0 - cell;
		}
/*		projection((glPushMatrix(), glLoadIdentity(), glOrtho(-1., 1., -1., 1., -1, 1)));*/
		glPushMatrix();
/*		glLoadIdentity();
		glTranslated(1. - MIN(gvp.h, gvp.w) / (double)gvp.w, 1. - MIN(gvp.h, gvp.w) / (double)gvp.h, 0.);
		glScaled(MIN(gvp.h, gvp.w) / (double)gvp.w, MIN(gvp.h, gvp.w) / (double)gvp.h, 1.);
		glTranslated(0., 0., 0.);*/
		MiniMapPos();

		glColor4ubv(hud_color_options[hud_color]);
		glPushAttrib(GL_TEXTURE_BIT);
		glBindTexture(GL_TEXTURE_2D, tex);
/*		glCallList(tex);*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glEnable(GL_TEXTURE_2D);
//		glDisable(GL_CULL_FACE);
		glBegin(GL_QUADS);
		glTexCoord2dv(tc[0]); glVertex2d(-1., -1.);
		glTexCoord2dv(tc[1]); glVertex2d(1., -1.);
		glTexCoord2dv(tc[2]); glVertex2d(1., 1.);
		glTexCoord2dv(tc[3]); glVertex2d(-1., 1.);
		glEnd();
		glPopAttrib();

		if(!pl.chase){
			entity_t *pt;
			amat4_t mat, mat2;
			glPushAttrib(GL_POINT_BIT);
			glPointSize(2);
			glDisable(GL_POINT_SMOOTH);
			glPushMatrix();
			MAT4IDENTITY(mat);
			MAT4ROTZ(mat2, mat, -angle);
			MAT4SCALE(mat2, 1. / g_maprange, 1. / g_maprange, 1.);
			MAT4TRANSLATE(mat2, pl.pos[0], pl.pos[2], 0.);
			glBegin(GL_POINTS);
			glColor4ub(255,0,0,255);
			for(pt = warf.tl; pt; pt = pt->next) if(pt->active){
				avec3_t v, v0;
				v0[0] = -pt->pos[0], v0[1] = -pt->pos[2], v0[2] = 0.;
				MAT4VP3(v, mat2, v0);
				if(-1. <= v[0] && v[0] <= 1. && -1. <= v[1] && v[1] <= 1.)
					glVertex3dv(v);
			}
			glEnd();
			glPopMatrix();
			glPopAttrib();
		}

		glColor4ubv(hud_color_options[hud_color]);

		glRasterPos3d(1., -1., 0.);
		gldprintf("range %lg km", g_maprange);

		/* center indicator */
		glBegin(GL_LINES);
		glVertex2d(-1., 0.);
		glVertex2d(1., 0.);
		glVertex2d(0., -1.);
		glVertex2d(0., 1.);
		glEnd();

		/* compass */
		glTranslated(-.7, .7, 0.);
		glRotated(-deg_per_rad * angle + 180, 0., 0., 1.);

		glBegin(GL_LINE_STRIP);
		glVertex2d(.05, .1);
		glVertex2d(-.05, .1);
		glVertex2d(.0, .15);
		glVertex2d(.0, -.1);
		glEnd();

		glBegin(GL_LINES);
		glVertex2d(-.1, 0.);
		glVertex2d(.1, 0.);
		glEnd();

/*		yaw = pl.pyr[1] + (pl.chase ? pl.chase->pyr[1] : 0.);
		glBegin(GL_LINES);
		glVertex2d(.5, .5);
		glVertex2d(.5 - .2 * cos(yaw), .5 + .2 * sin(yaw));
		glVertex2d(.5, .4);
		glVertex2d(.5, .6);
		glVertex2d(.4, .5);
		glVertex2d(.6, .5);
		glEnd();*/
		glPopMatrix();
/*		projection(glPopMatrix());*/
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}
#endif
