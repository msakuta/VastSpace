#include "tent3d.h"
#include "tent3d_p.h"
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquat.h>
#include <clib/suf/suf.h>
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
/*#include <GL/glut.h>*/

/* teline_flags_t */
#define TEL_FORMS	(TEL_CIRCLE|(TEL_CIRCLE<<1)|(TEL_CIRCLE<<2)|(TEL_CIRCLE<<3)|(TEL_CIRCLE<<4)) /* bitmask for forms */

/* Rotation private flags. since rotation needs matrix multiplication and
  trigonometric operations for each axis, it's not good idea to rotate even
  totally-nonrotating objects. instead, save potential rotatabilities for
  each axes and rotate around only relevant axes.
   This all should not be caller-aware, since angles and angle velocities are
  enough information for this process given by the caller. */
#define TEL3_ROTATEX (1<<29) 
#define TEL3_ROTATEY (1<<30)
#define TEL3_ROTATEZ (1<<31)
#define TEL3_ROTATE (TEL3_ROTATEX | TEL3_ROTATEY | TEL3_ROTATEZ)

#define FADE_START .5
#define SHRINK_START .3

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#define rad2deg (360. / 2. / M_PI)

#define VECNULL(r) ((r)[0]=(r)[1]=(r)[2]=0.)
struct vectorhack{
	double d[3];
};
#define VECCPY(r,a) ((r)[0]=(a)[0],(r)[1]=(a)[1],(r)[2]=(a)[2])/*(*(struct vectorhack*)&(r)=*(struct vectorhack*)&(a))*/
#define VECSADD(r,b,s) ((r)[0]+=(b)[0]*(s),(r)[1]+=(b)[1]*(s),(r)[2]+=(b)[2]*(s))
#define VECSCALE(r,a,s) ((r)[0]=(a)[0]*(s),(r)[1]=(a)[1]*(s),(r)[2]=(a)[2]*(s))
#define VECSLEN(a) ((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2])
#define VECLEN(a) sqrt((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2])

#define COLISTRGB(c) COLOR32R(c), COLOR32G(c), COLOR32B(c)
#define COLIST(c) COLOR32R(c), COLOR32G(c), COLOR32B(c), COLOR32A(c)

typedef struct tent3d_line{
	double pos[3]; /* center point, double because the velo may be so slow */
	double velo[3]; /* velocity. */
	double len; /* length of the beam */
/*	double pyr[3];*/ /* pitch, yaw and roll angles of the beam */
	double rot[4]; /* quit using euler angles */
	double omg[3]; /* angle velocity */
	double life; /* remaining rendering time */
	double grv[3]; /* gravity effect factor */
	tent3d_flags_t flags; /* entity options */
	struct tent3d_line *next; /* list next pointer */
	union{ /* rendering model, which depends on flags & TEL_FORMS */
		COLOR32 r; /* color of the beam, black means random per rendering */
		const struct color_sequence *cs; /* TEL_GRADCIRCLE */
		const struct cs2 *cs2; /* TEL_GRADCIRCLE2 */
		suf_t *suf;
		struct{void (*f)(const struct tent3d_line_callback *, const struct tent3d_line_drawdata*, void*); void *p;} callback;
		struct{COLOR32 r; float maxlife;} rm; /* certain forms needs its initial life time */
	} mdl;
} teline3_t;

/* this is a pointer list pool, which does not use heap allocation or shifting
  during the simulation which can be serious overhead. */
typedef struct tent3d_line_list{
	unsigned m; /* cap for total allocations of elements */
	unsigned ml; /* allocated elements in l */
	unsigned mex; /* allocated elements in an extra list */
	teline3_t *l;
	teline3_t *lfree, *lactv, *last;
	struct tent3d_line_extra_list{
		struct tent3d_line_extra_list *next;
		teline3_t l[1]; /* variable */
	} *ex; /* extra space is allocated when entities hits their max count */
	unsigned bs; /* verbose buffer size */
#ifndef NPROFILE
	struct tent3d_line_debug debug;
#endif
} tell_t;



/* allocator and constructor */
tell_t *NewTeline3D(unsigned maxt, unsigned init, unsigned unit){
	/* it's silly that max is defined as a macro */
	int i;
	tell_t *ret;
	ret = malloc(sizeof *ret);
	if(!ret) return NULL;
	ret->lfree = ret->l = malloc(init * sizeof *ret->l);
	if(!ret->l) return NULL;
	ret->m = maxt;
	ret->ml = ret->bs = init;
	ret->mex = unit;
	ret->lactv = ret->last = NULL;
	ret->ex = NULL;
	for(i = 0; i < (int)(init - 1); i++)
		ret->l[i].next = &ret->l[i+1];
	if(init) ret->l[init-1].next = NULL;
#ifndef NDEBUG
	ret->debug.so_teline3_t = sizeof(teline3_t);
	ret->debug.teline_m = ret->bs;
	ret->debug.teline_c = 0;
	ret->debug.teline_s = 0;
#endif
	return ret;
}

/*
tell_t *NewTeline3DFunc(unsigned maxt, unsigned init, unsigned unit, warf_t *w){
	tell_t *ret;
	ret = NewTeline3D(maxt, init, unit);
	ret->w = w;
	return ret;
}
*/
/* destructor and deallocator */
void DelTeline3D(tell_t *p){
	if(p->l) free(p->l);
	if(p->ex){
		struct tent3d_line_extra_list *ex = p->ex, *nex;
		while(ex){
			nex = ex->next;
			free(ex);
			ex = nex;
		}
	}
	free(p);
}

static teline3_t *alloc_teline(tell_t *p, const double pos[3], const double velo[3],
					double len, const double pyr[3], const double omg[3], const double grv[3],
					tent3d_flags_t flags, double life)
{
	teline3_t *pl;
	if(!p || !p->m) return NULL;
	pl = p->lfree;
	if(!pl){
		if(p->mex && p->bs + p->mex <= p->m){
			struct tent3d_line_extra_list *ex;
			unsigned i;
			ex = malloc(offsetof(struct tent3d_line_extra_list, l) + p->mex * sizeof *ex->l); /* struct hack alloc */
			ex->next = p->ex;
			p->ex = ex;
			p->lfree = ex->l;
			for(i = 0; i < p->mex-1; i++)
				ex->l[i].next = &ex->l[i+1];
			ex->l[p->mex-1].next = NULL;
			pl = p->lfree;
			p->bs += p->mex;
#ifndef NDEBUG
			p->debug.teline_m = p->bs;
#endif
		}
		else{
			pl = p->lactv; /* reuse the oldest active node */
			if(!pl) return NULL;

			/* roll pointers */
			p->last->next = p->lactv;
			p->last = p->lactv;
			p->lactv = p->lactv->next;
			pl->next = NULL;
		}
	}
	else{ /* allocate from the free list. */
		p->lfree = pl->next;
		pl->next = p->lactv;
		p->lactv = pl;
		if(!pl->next) p->last = pl;
	}

	/* filter these bits a little */
	if(pyr || omg){
		tent3d_flags_t x, y, z;
		x = (pyr ? pyr[0] : 0) || (omg ? omg[0] : 0) ? TEL3_ROTATEX : 0;
		y = (pyr ? pyr[1] : 0) || (omg ? omg[1] : 0) ? TEL3_ROTATEY : 0;
		z = (pyr ? pyr[2] : 0) || (omg ? omg[2] : 0) ? TEL3_ROTATEZ : 0;
		flags &= ~TEL3_ROTATE; /* truncate any given flags */
		flags |= x | y | z;
	}

#define tel (*pl)
	if(pos) VECCPY(pl->pos, pos); else VECNULL(pl->pos);
	if(velo) VECCPY(pl->velo, velo); else VECNULL(pl->velo);
	tel.len = len;
	if(pyr){
/*		if(flags & TEL3_QUAT){*/
			QUATCPY(pl->rot, pyr);
/*		}
		else{
			pyr2quat(pl->rot, pyr);
		}*/
	}
	else{
		VECNULL(pl->rot);
		pl->rot[3] = 1.;
	}
	if(omg) VECCPY(tel.omg, omg); else VECNULL(pl->omg);
	if(grv) VECCPY(pl->grv, grv); else VECNULL(pl->grv);
	tel.flags = flags;
	tel.life = life;
#undef tel

#ifndef NDEBUG
	p->debug.teline_s++;
#endif
	return pl;
}

void AddTeline3D(tell_t *p, const double pos[3], const double velo[3],
	double len, const double pyr[3], const double omg[3], const double grv[3],
	COLOR32 col, tent3d_flags_t flags, double life)
{
	teline3_t *pl;
	pl = alloc_teline(p, pos, velo, len, pyr, omg, grv, (flags & TEL_FORMS) == TEL3_CALLBACK ? flags & ~TEL_FORMS : flags, life);
	if(!pl) return;
	pl->mdl.r = col;
	switch(flags & TEL_FORMS){
		case TEL3_CYLINDER:
		case TEL3_EXPANDISK:
		case TEL3_EXPANDGLOW:
		case TEL3_EXPANDTORUS:
			pl->mdl.rm.maxlife = life;
	}
}

void AddTelineCallback3D(tell_t *p, const double pos[3], const double velo[3],
	double len, const double pyr[3], const double omg[3], const double grv[3],
	void (*f)(const struct tent3d_line_callback*, const struct tent3d_line_drawdata*, void*), void *pd, tent3d_flags_t flags, double life)
{
	teline3_t *pl;
	pl = alloc_teline(p, pos, velo, len, pyr, omg, grv, flags |= TEL3_CALLBACK | TEL3_NOLINE, life);
	if(!pl) return;
	pl->mdl.callback.f = f;
	pl->mdl.callback.p = pd;
}

/* animate every line */
void AnimTeline3D(tell_t *p, double dt){
	teline3_t *pl = p->lactv, **ppl = &p->lactv;
	while(pl){
		teline3_t *pl2;
		pl->life -= dt;

		/* delete condition */
		if(pl->life < 0){
			/* roll pointers */
			if(pl == p->last && *ppl != p->lactv)
				p->last = (teline3_t*)((char*)ppl - offsetof(teline3_t, next));
			pl2 = pl->next;
			*ppl = pl->next;
			pl->next = p->lfree;
			p->lfree = pl;
		}
		else{
			ppl = &pl->next;
			pl2 = pl->next;
		}
#if ENABLE_FOLLOW
		if(!pl->flags & TEL_FOLLOW){
#endif
/*		if(pl->flags & TEL3_ACCELFUNC && p->w && p->w->vft->accel){
			avec3_t accel;
			p->w->vft->accel(p->w, &accel, &pl->pos, &pl->velo);
			VECSADD(pl->velo, accel, dt);
		}*/
		VECSADD(pl->velo, pl->grv, dt); /* apply gravity */
		VECSADD(pl->pos, pl->velo, dt);

		if(!(pl->flags & TEL3_HEADFORWARD)){
			aquat_t q, qomg;
			qomg[3] = 0.;
			VECSCALE(qomg, pl->omg, dt / 2.);
			QUATMUL(q, qomg, pl->rot);
			VEC4ADDIN(pl->rot, q);
			QUATNORMIN(pl->rot);
/*			VECSADD(pl->pyr, pl->omg, dt);*/
		}

#if 0
		/* rough approximation of reflection. rigid body dynamics are required for more reality. */
		if(pl->flags & TEL3_REFLECT){
/*			if(pl->flags & TEL3_HITFUNC){
				struct contact_info ci;
				if(p->w && p->w->vft->pointhit && p->w->vft->pointhit(p->w, &pl->pos, &pl->velo, dt, &ci)){
					double sp;
					sp = (1. + REFLACTION) * -VECSP(ci.normal, pl->velo);
					VECSADD(pl->velo, ci.normal, sp);
				}
			}
			else*/{
				int j = 0; /* reflected at least once */

				/* floor reflection only! */
				if(pl->pos[1] + pl->velo[1] * dt < 0)
					pl->velo[1] *= -REFLACTION, j = 1;

				if(j && !(pl->flags & (TEL3_HEADFORWARD)))
					pl->omg[1] *= - REFLACTION;
			}
		}
#endif

		/* redirect its head to where the line going to go */
/*		if(pl->flags & TEL_HEADFORWARD)
			pl->ang = atan2(pl->vy, pl->vx) + pl->omg;
		else if(!(pl->flags & TEL_FORMS))
			pl->ang = fmod(pl->ang + pl->omg * dt, 2 * M_PI);*/

		pl = pl2;
	}
#ifndef NDEBUG
	if(p->ml){
		unsigned int i = 0;
		teline3_t *pl = p->lactv;
		while(pl) i++, pl = pl->next;
		p->debug.teline_c = i;
	}
#endif
}

/* since 3d graphics have far more parameters than that of 2d, we pack those variables
  to single structure to improve performance of function calls. */
void DrawTeline3D(tell_t *p, struct tent3d_line_drawdata *dd){
	timemeas_t tm;
	TimeMeasStart(&tm);
	if(!p)
		return;
	{
	double br, lenb;
	unsigned char buf[4];
	teline3_t *pl = p->lactv;

	while(pl){
		tent3d_flags_t form = pl->flags & TEL_FORMS;
		double fore[4];
		double *rot = pl->flags & TEL3_HEADFORWARD ? fore : pl->rot;
		COLOR32 col;
		br = MIN(pl->life / FADE_START, 1.0);
		col = pl->life < FADE_START ? (pl->mdl.r & 0x00ffffff) | COLOR32RGBA(0,0,0,(GLubyte)(255 * pl->life / FADE_START)) : pl->mdl.r;

		if(pl->flags & TEL3_HEADFORWARD){
			avec3_t dr, v;
			aquat_t q;
			amat4_t mat;
			double p;
			VECNORM(dr, pl->velo);

			/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
			q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

			VECVP(v, avec3_001, dr);
			p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
			VECSCALE(q, v, p);
			QUATMUL(fore, q, pl->rot);

/*			fore[0] = pl->pyr[0] - rad2deg * atan2(pl->velo[1], sqrt(pl->velo[0] * pl->velo[0] + pl->velo[2] * pl->velo[2]));
			fore[1] = pl->pyr[1] + rad2deg * atan2(pl->velo[0], pl->velo[2]);
			fore[2] = pl->pyr[2];*/
		}

		/* shrink over its lifetime */
		if(pl->flags & TEL_SHRINK && pl->life < SHRINK_START)
			lenb = pl->len / SHRINK_START * pl->life;
		else
			lenb = pl->len;

		/* recalc length by its velocity. */
		if(pl->flags & TEL_VELOLEN)
			lenb *= VECLEN(pl->velo);

		if(form == TEL3_EXPANDTORUS){
			int i;
			COLOR32 cc, ce; /* center color, edge color */
			double (*cuts)[2], radius;
			struct gldBeamsData bd;
			radius = (pl->mdl.rm.maxlife - pl->life) * lenb;
			cuts = CircleCuts(16);
			glColor4ub(COLIST(col));
			bd.cc = 0;
			for(i = 0; i <= 16; i++){
				int k = i % 16;
				double pos[3];
				pos[0] = pl->pos[0] + cuts[k][0] * radius;
				pos[1] = pl->pos[1];
				pos[2] = pl->pos[2] + cuts[k][1] * radius;
				gldBeams(&bd, dd->viewpoint, pos, radius / 10., col);
			}
		}
		else if(form && form != TEL3_CALLBACK){
			glPushMatrix();
			if(pl->flags & TEL3_NEAR){
				GLdouble mat[16];
				static const GLdouble mat2[16] = {
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,1.1, /* a bit near */
				};
				glGetDoublev(GL_MODELVIEW_MATRIX, mat);
				glLoadMatrixd(mat2);
				glMultMatrixd(mat);
			}
			glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
/*			if(pl->flags & TEL3_ROTATE){
				if(pl->flags & TEL3_ROTATEY) glRotated(pl->pyr[1] * deg_per_rad, 0.0, -1.0, 0.0);
				if(pl->flags & TEL3_ROTATEX) glRotated(pl->pyr[0] * deg_per_rad, 1.0, 0.0, 0.0);
				if(pl->flags & TEL3_ROTATEZ) glRotated(pl->pyr[2] * deg_per_rad, 0.0, 0.0, 1.0);
			}*/
			{
				amat4_t mat;
				quat2mat(&mat, rot);
				glMultMatrixd(mat);
			}
			if(pl->flags & TEL3_INVROTATE)
				glMultMatrixd(dd->invrot);

			if(form == TEL3_PSEUDOSPHERE){
				GLubyte col[4];
				COLOR32UNPACK(col, pl->mdl.r);
				col[3] = pl->life < FADE_START ? COLOR32A(pl->mdl.r) * pl->life / FADE_START : COLOR32A(pl->mdl.r);
				gldPseudoSphere(pl->pos, lenb, col);
			}
			else if(form == TEL3_CYLINDER){
				int i;
				GLubyte cc[4], ce[4];
				double (*cuts)[2], radius;
				ce[0] = cc[0] = COLOR32R(col);
				ce[1] = cc[1] = COLOR32G(col);
				ce[2] = cc[2] = COLOR32B(col);
				cc[3] = COLOR32A(col);
				ce[3] = 0;
				cuts = CircleCuts(5);
				radius = (pl->mdl.rm.maxlife - pl->life) * lenb;
				glBegin(GL_QUAD_STRIP);
				for(i = 0; i <= 5; i++){
					int k = i % 5;
					glColor4ubv(cc);
					glVertex3d(cuts[k][0] * radius, cuts[k][2] * radius, 0.);
					glColor4ubv(ce);
					glVertex3d(cuts[k][0] * radius, cuts[k][2] * radius, radius);
				}
				glEnd();
			}
			else if(form == TEL3_EXPANDISK || form == TEL3_STATICDISK){
				int i;
				COLOR32 cc, ce; /* center color, edge color */
				double (*cuts)[2], radius;
				if(form == TEL3_EXPANDISK){
					cc = col & 0x00ffffff;
					ce = col;
					radius = (pl->mdl.rm.maxlife - pl->life) * lenb;
				}
				else{
					cc = col;
					ce = col & 0x00ffffff;
					radius = lenb;
				}
				cuts = CircleCuts(16);
				glBegin(GL_TRIANGLE_FAN);
				glColor4ub(COLIST(cc));
				glVertex3d(0., 0., 0.);
				glColor4ub(COLIST(ce));
				for(i = 0; i <= 16; i++){
					int k = i % 16;
					glVertex3d(cuts[k][0] * radius, cuts[k][1] * radius, 0.);
				}
				glEnd();
			}
			else if(form == TEL3_GLOW || form == TEL3_EXPANDGLOW){
				int i;
				double (*cuts)[2];
				double w = .1;
				cuts = CircleCuts(10);
				if(form == TEL3_EXPANDGLOW)
					lenb *= (pl->mdl.rm.maxlife - pl->life);
				glScaled(lenb, lenb, lenb);
				glBegin(GL_TRIANGLE_FAN);
				glColor4ub(COLIST(col));
				glVertex3d(0., 0., 0.);
				glColor4ub(COLISTRGB(col), 0);
				for(i = 0; i <= 10; i++){
					int k = i % 10;
					glVertex4d(w * cuts[k][1], w * cuts[k][0], 0., w);
				}
				glEnd();
			}
			else if(form == TEL3_SPRITE){
				int i;
				double (*cuts)[2];
				cuts = CircleCuts(10);
				glScaled(lenb, lenb, lenb);
				glBegin(GL_TRIANGLE_FAN);
				glColor4ub(COLIST(col));
				glVertex3d(0., 0., 0.);
				glColor4ub(COLISTRGB(col), 0);
				for(i = 0; i <= 10; i++){
					int k = i % 10;
					glVertex3d(cuts[k][1], cuts[k][0], 0.);
				}
				glEnd();
			}
			else{ /* if(forms == TEL_POINT) */
				glBegin(GL_POINTS);
				glVertex3dv(pl->pos);
				glEnd();
			}
			glPopMatrix();
		}
		else if(form == TEL3_CALLBACK){
			pl->mdl.callback.f((struct tent3d_line_callback*)pl, dd, pl->mdl.callback.p);
		}

		if(!(pl->flags & TEL3_NOLINE)){
#if 0
			glColor4ub(COLIST(col));
			glPushMatrix();
			glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
			glRotated(pyr[2], 0., 0., 1.);
			glRotated(pyr[1], 0., 1., 0.);
			glRotated(pyr[0], 1., 0., 0.);
			glScaled(lenb, lenb, lenb);

			glBegin(GL_LINES);
			glVertex3d(0., 0., -1.);
			glVertex3d(0., 0.,  1.);
			glEnd();
			glPopMatrix();
#else
#if 0
			if(ppyr){
				static const double rotaxis[16] = {
					1,0,0,0,
					0,0,1,0,
					0,-1,0,0,
					0,0,0,1,
				};
				GLubyte cc[4] = {COLIST(col)}, oc[4] = {COLISTRGB(col), 0};
				glPushMatrix();

				glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
				glRotated(pyr[1], 0., 1., 0.);
				glRotated(pyr[0], 1., 0., 0.);
/*				glRotated(ppyr[1] * rad2deg, 0., -1., 0.);*/
				glScaled(lenb / 5, 1., lenb);
				glMultMatrixd(rotaxis);

				glBegin(GL_QUAD_STRIP);
				glColor4ubv(oc);
				glVertex3d(-1., 0., 0.);
				glVertex3d(-1., 1., 0.);
				glColor4ubv(cc);
				glVertex3d( 0., 0., 0.);
				glVertex3d( 0., 1., 0.);
				glColor4ubv(oc);
				glVertex3d( 1., 0., 0.);
				glVertex3d( 1., 1., 0.);
				glEnd();
				glPopMatrix();
			}
#endif

			{
			double start[3], end[3];
			double width = pl->flags & TEL3_HEADFORWARD ? .002 : lenb * .2;
			if(pl->flags & TEL3_HEADFORWARD){
				lenb /= VECLEN(pl->velo);
				start[0] = pl->pos[0] - pl->velo[0] * lenb;
				start[1] = pl->pos[1] - pl->velo[1] * lenb;
				start[2] = pl->pos[2] - pl->velo[2] * lenb;
				end[0] = pl->pos[0] + pl->velo[0] * lenb;
				end[1] = pl->pos[1] + pl->velo[1] * lenb;
				end[2] = pl->pos[2] + pl->velo[2] * lenb;
			}
			else{
				amat4_t mat;
				quat2mat(&mat, rot);
				mat4dvp3(start, mat, avec3_001);
				VECSCALEIN(start, lenb);
				VECSCALE(end, start, -1);
				VECADDIN(start, pl->pos);
				VECADDIN(end, pl->pos);
/*				start[0] = pl->pos[0] - pl->pyr[0] * lenb;
				start[1] = pl->pos[1] - pl->pyr[1] * lenb;
				start[2] = pl->pos[2] - pl->pyr[2] * lenb;
				end[0] = pl->pos[0] + pl->pyr[0] * lenb;
				end[1] = pl->pos[1] + pl->pyr[1] * lenb;
				end[2] = pl->pos[2] + pl->pyr[2] * lenb;*/
			}

			if(pl->flags & TEL3_THICK && VECSDIST(dd->viewpoint, pl->pos) < /*width * width * 1e-3 * 1e-3 /*/.5 * .5){
				if(pl->flags & TEL3_FADEEND){
					if(pl->flags & TEL3_FADEEND)
						glColor4ub(COLISTRGB(col), 0);
					else
						glColor4ub(COLIST(col));
					gldGradBeam(dd->viewpoint, start, end, width, col);
				}
				else
					gldBeam(dd->viewpoint, start, end, width);
			}
			else{
				glBegin(GL_LINES);
				if(pl->flags & TEL3_FADEEND)
					glColor4ub(COLISTRGB(col), 0);
				else
					glColor4ub(COLIST(col));
				glVertex3dv(start);
				if(pl->flags & TEL3_FADEEND)
					glColor4ub(COLIST(col));
				glVertex3dv(end);
				glEnd();
			}
			}
#endif
		}

		pl = pl->next;
	}
	}
#ifndef NPROFILE
	p->debug.drawteline = TimeMeasLap(&tm);
#endif
}



#ifndef NPROFILE
const struct tent3d_line_debug *Teline3DDebug(const tell_t *p){
	return &p->debug;
}
#endif

