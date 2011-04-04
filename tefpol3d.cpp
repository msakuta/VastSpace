#include "tent3d.h"
#include "tent3d_p.h"
extern "C"{
#include <clib/avec3.h>
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/cull.h>
#include <clib/rseq.h>
}
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
/*#include <GL/glut.h>*/

/*#############################################################################
	macro definitions
#############################################################################*/
#define DRAWORDER 0 /* whether draw from the head to the tail, otherwise reverse */
#define ENABLE_FORMS 0 /* head forms */
#define ENABLE_FINE 0 /* granularity control for line graduation */
#define ENABLE_ROUGH 0 /* less fine */
#define ENABLE_RETURN 0 /* return from opposite edge, doesn't make sense for polylines */
#define ENABLE_THICK 1 /* thickness control in conjunction with WinAPI drawing */
#define ENABLE_HEAD 0 /* head sprite */
#define INTERP_CS 1 /* Liner interpolation rather than calling ColorSequence() everytime */
#define VERTEX_SIZE 0x8000

/*#if !WINTEFPOL && !DIRECTDRAW
#undef ENABLE_THICK
#define ENABLE_THICK 0
#endif*/

#define TEP3_THICKNESS (TEP3_THICK|(TEP3_THICK<<1)|(TEP3_THICK<<2))
#define THICK_BIT 7
#define TEP_BLENDMASK (TEP_SOLIDBLEND|(TEP_SOLIDBLEND<<1))
#define TEP3_MOVABLE (1<<16) /* cannot be freed unless explicitly specified by the caller */
#define TEP3_SKIP    (1<<17) /* skip generation of vertex for movable tefpol */

#ifndef ABS
#define ABS(a) (0<(a)?(a):-(a))
#endif

/*=============================================================================
	type definitions
=============================================================================*/
typedef struct color_sequence colseq_t;

/* a set of auto-decaying line segments */
typedef struct tent3d_fpol{
	double pos[3];
	double velo[3];
	double life;
	double grv[3];
	const colseq_t *cs;
	unsigned long flags; /* entity options */
	unsigned cnt; /* generated trail node count */
	struct te_vertex *head, *tail; /* polyline's */
	struct tent3d_fpol *next;
} tefpol_t;

/* a tefpol can have arbitrary number of vertices in the vertices buffer. */
typedef struct te_vertex{
	double pos[3];
	float dt;
	struct te_vertex *next;
} tevert_t;

struct te_fpol_extra_list{
	struct te_fpol_extra_list *next;
	tefpol_t l[1]; /* variable */
};

typedef struct tent3d_fpol_list{
	unsigned m;
	unsigned ml;
	unsigned mex;
	tefpol_t *l;
	tefpol_t *lfree, *lactv, *last;
	te_fpol_extra_list *ex;
	unsigned bs; /* verbose buffer size */
#ifndef NDEBUG
	struct tent3d_fpol_debug debug;
#endif
} tepl_t;

/* global vertice pool, used by multiple entities */
static struct te_vertice_list{
	unsigned m;
	tevert_t *l;
	tevert_t *lfree, *lfst;
} svl = {0, NULL};


/*-----------------------------------------------------------------------------
	function definitions
-----------------------------------------------------------------------------*/

#if 1
#define checkVertices(a)
#else
static void checkVertices(tepl_t *p){
	tefpol_t *pl = p->lactv;
	static int invokes = 0;
	char *ok;
	ok = calloc(sizeof*ok, svl.m);
	while(pl){ /* check all tefpols */
		tevert_t *pv = pl->head;
		while(pv){
			ok[pv - svl.l] = 1;
			if(!pv->next) assert(pl->tail == pv);
			pv = pv->next;
		}
		pl = pl->next;
	}
	{/* check free list */
		tevert_t *pv = svl.lfree;
		while(pv){
			ok[pv - svl.l] = 1;
			pv = pv->next;
		}
	}
	{
		int i;
		for(i = 0; i < svl.m; i++)
			assert(ok[i]);
	}
	free(ok);
	invokes++;
}
#endif

/* free vertex nodes between *pp and plast */
static void freeVertex(tevert_t **pp, tevert_t *plast){
/*	if((*pp)->next) freeVertex(&(*pp)->next);
	(*pp)->next = svl.lfree;
	svl.lfree = *pp;
	if(!svl.lfree->next) svl.lfst = *pp;
	*pp = NULL;
*/
#if 1 /* put back to head */
	if(*pp){
		tevert_t *pfirst = *pp;
		assert(!svl.lfree == !svl.lfst);
		assert(!svl.lfst || !svl.lfst->next);
		assert(!plast || !plast->next);
		if(plast)
			*pp = plast->next, plast->next = svl.lfree;
		else{
			tevert_t *p = pfirst;
			while(p->next) p = p->next;
			(plast = p)->next = svl.lfree;
		}
		if(!svl.lfree) svl.lfst = plast;
		svl.lfree = pfirst;
	}
	assert(!svl.lfree == !svl.lfst);
	assert(!svl.lfst || !svl.lfst->next);
#else /* push back from back */
	if(*pp){
		assert(!svl.lfree == !svl.lfst);
		assert(!svl.lfst || !svl.lfst->next);
		assert(!plast || !plast->next);
		if(svl.lfree){
			svl.lfst->next = *pp;
		}
		else{
			svl.lfree = *pp;
		}
		if(plast) svl.lfst = plast;
		else{
			tevert_t *p = *pp;
			while(p->next) p = p->next;
			svl.lfst = p;
		}
		*pp = NULL;
	}
#endif
}

static void freeTefpolExtra(struct te_fpol_extra_list *p){
	if(p->next) freeTefpolExtra(p->next);
	free(p);
}

tent3d_fpol_list *NewTefpol3D(unsigned maxt, unsigned init, unsigned unit){
	int i;
	tepl_t *ret;
	ret = new tepl_t;
	if(!ret) return NULL;
	ret->lfree = ret->l = (tefpol_t*)malloc(init * sizeof *ret->l);
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
	ret->debug.so_tefpol3_t = sizeof(tefpol_t);
	ret->debug.so_tevert3_t = sizeof(tevert_t);
	ret->debug.tefpol_m = ret->bs;
	ret->debug.tefpol_c = 0;
	ret->debug.tefpol_s = 0;
	ret->debug.tevert_c = 0;
	ret->debug.tevert_s = 0;
#endif
	return ret;
}

/* destructor and deallocator */
void DelTefpol(tepl_t *p){
	if(p->l){
		tefpol_t *pl = p->lactv;
		while(pl){
			freeVertex(&pl->head, pl->tail);
			pl = pl->next;
		}
		free(p->l);
	}
	if(p->ex) freeTefpolExtra(p->ex);
	delete(p);
}

static tefpol_t *allocTefpol3D(tepl_t *p, const double pos[3], const double velo[3],
			   const double grv[3], const colseq_t *col, tent3d_flags_t f, double life)
{

	tefpol_t *pl;
	if(!p || !p->m) return NULL;
	pl = p->lfree;
	if(!pl){
		if(p->mex && p->bs + p->mex <= p->m){
			struct te_fpol_extra_list *ex;
			unsigned i;
			ex = (te_fpol_extra_list*)malloc(offsetof(struct te_fpol_extra_list, l) + p->mex * sizeof *ex->l); /* struct hack alloc */
			ex->next = p->ex;
			p->ex = ex;
			p->lfree = ex->l;
			for(i = 0; i < p->mex-1; i++)
				ex->l[i].next = &ex->l[i+1];
			ex->l[p->mex-1].next = NULL;
			pl = p->lfree;
			p->bs += p->mex;
#ifndef NDEBUG
			p->debug.tefpol_m = p->bs;
#endif
		}
		else{
			tefpol_t **ppl = &p->lactv; /* reuse the oldest active node */
			while(*ppl && (*ppl)->flags & TEP3_MOVABLE) ppl = &(*ppl)->next; /* skip movable nodes */
			if(!*ppl || *ppl == p->last) return NULL;
			pl = *ppl;
			assert(!pl->tail || !pl->tail->next);

			/* free vertices */
			freeVertex(&pl->head, pl->tail);
/*			if(pl->head){
				if(svl.lfree){
					svl.lfst->next = pl->head;
					svl.lfst = pl->tail;
				}
				else{
					svl.lfree = pl->head;
					svl.lfst = pl->tail;
				}
			}*/

			/* roll pointers */
			p->last->next = pl;
			p->last = pl;
			*ppl = pl->next;
			pl->next = NULL;
		}
	}
	else{ /* allocate from the free list. */
		p->lfree = pl->next;
		pl->next = p->lactv;
		p->lactv = pl;
		if(!pl->next) p->last = pl;
	}
	/* filter these bits a little. */
//	if(flags & TEL_NOLINE) flags &= ~TEL_HEADFORWARD;
//	if((flags & TEL_FORMS) == TEL_POINT) flags &= ~TEL_VELOLEN;

	VECCPY(pl->pos, pos);
	VECCPY(pl->velo, velo);
	VECCPY(pl->grv, grv);
	pl->cs = col;
	pl->flags = f;
	pl->cnt = 0;
	pl->life = life;
	pl->head = pl->tail = NULL;

#ifndef NDEBUG
	p->debug.tefpol_s++;
#endif
	return pl;
}

void AddTefpol3D(tepl_t *p, const double pos[3], const double velo[3],
			   const double grv[3], const colseq_t *col, tent3d_flags_t f, double life)
{
	allocTefpol3D(p, pos, velo, grv, col, f & ~(TEP3_MOVABLE | TEP3_SKIP), life);
}

tefpol_t *AddTefpolMovable3D(tepl_t *p, const double pos[3], const double velo[3],
			   const double grv[3], const colseq_t *col, tent3d_flags_t f, double life)
{
	return allocTefpol3D(p, pos, velo, grv, col, f | TEP3_MOVABLE, life);
}

void MoveTefpol3D(tefpol_t *pl, const double pos[3], const double velo[3], const double life, int skip){
	if(!pl || !(pl->flags & TEP3_MOVABLE))
		return;
	VECCPY(pl->pos, pos);
	if(velo){ VECCPY(pl->velo, velo); }
	else{ VECNULL(pl->velo); }
	pl->life = life;
	if(skip)
		pl->flags |= TEP3_SKIP;
	else
		pl->flags &= ~TEP3_SKIP;
}

void ImmobilizeTefpol3D(tefpol_t *pl){
	pl->flags &= ~TEP3_MOVABLE;
}

void AnimTefpol3D(tepl_t *p, double dt){
#ifndef NDEBUG
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
#endif
	tefpol_t *pl = p->lactv, **ppl = &p->lactv;

	while(pl){
		tefpol_t *pl2;
		int genvert = 0; /* whether a vertex is generated */
#	if ENABLE_THICK
		tent3d_flags_t thickness = pl->flags & TEP3_THICKNESS;
		double width =
			thickness == TEP3_THICK ? .001 :
			thickness == TEP3_THICKER ? .002 : 
			thickness == TEP3_FAINT ? .0005 : .005;
#	else
		tent3d_flags_t thickness = 0;
		double width = 0.;
#	endif


		/* delete condition */
		/* if no border behavior is specified, it should be deleted */
		if(!(pl->flags & TEP3_MOVABLE) && pl->life < -pl->cs->t){

			/* free occupied vertices */
			freeVertex(&pl->head, pl->tail);
			pl->tail = NULL;

			/* roll pointers */
			if(pl == p->last && *ppl != p->lactv)
				p->last = (tefpol_t*)((byte*)ppl - offsetof(tefpol_t, next));
			pl2 = pl->next;
			*ppl = pl->next;
			pl->next = p->lfree;
			p->lfree = pl;
			checkVertices(p);
			pl = pl2;
			continue;
		}
		else{
			ppl = &pl->next;
			pl2 = pl->next;
		}
		if(!svl.l){ /* allocate the vertex buffer */
			int i;
			svl.l = svl.lfree = (tevert_t*)malloc((svl.m = VERTEX_SIZE) * sizeof *svl.l);
			for(i = 0; i < svl.m-1; i++)
				svl.l[i].next = &svl.l[i+1];
			if(svl.m){
				svl.l[i].next = NULL;
				svl.lfst = &svl.l[i];
			}
		}
		{ /* free outdated vertices */
#if DRAWORDER
		tevert_t **ppv = &pl->head;
		double t = 0;
		while(*ppv){
			if(pl->cs->t < (t += ABS((*ppv)->dt))){
				freeVertex(ppv, pl->tail);
				if(ppv != &pl->head)
					pl->tail = (tevert_t*)((byte*)ppv - offsetof(struct te_vertex, next));
				else
					pl->tail = NULL;
				break;
			}
			ppv = &(*ppv)->next;
		}
#else
		tevert_t *pv = pl->head;
		double t = 0;
		for(; pv; pv = pv->next) t += ABS(pv->dt);
		while(pl->cs->t < t){ /* delete from head */
			pv = pl->head;
			pl->head = pl->head->next;
			pv->next = svl.lfree;
			svl.lfree = pv;
			if(!pl->head) pl->tail = NULL;
			if(!svl.lfree->next) svl.lfst = svl.lfree;
			t -= ABS(pv->dt);
		}
#endif
		checkVertices(p);
		}
		/* AddVertex: add a vertex for the polyline */
		if(pl->flags & TEP3_SKIP){
#if DRAWORDER
			if(pl->head){
				VECCPY(pl->head->pos, pl->pos);
				pl->head->dt = -ABS(pl->head->dt) - dt;
			}
#else
			if(pl->tail){
				VECCPY(pl->tail->pos, pl->pos);
				pl->tail->dt = -ABS(pl->tail->dt) - dt;
			}
#endif
		}
		else if(genvert = pl->life > 0 && (!pl->head || !pl->tail || (thickness ? width * width < VECSDIST(pl->pos, pl->tail->pos) : !VECEQ(pl->pos, pl->head->pos)))){
		pl->cnt++;
		if(!(pl->flags & TEP3_ROUGH) || pl->cnt % 2){
		tevert_t *pv = svl.lfree;
		if(!pv){
			pv = pl->head; /* RerollVertex: reuse the oldest active node */
			if(!pv) goto novertice;

			/* roll pointers */
#if /*DRAWORDER*/0
			pl->tail->next = pl->head;
			pl->head = pl->tail;
			for(pv = pl->head; pv->next != pl->tail; pv = pv->next);
			pl->tail = pv;
			pv->next = NULL;
#else
			pl->tail->next = pl->head;
			pl->tail = pl->head;
			pl->head = pl->head->next;
			pv->next = NULL;
#endif
			checkVertices(p);
		}
		else{ /* AllocVertex: allocate from the pool */
#if DRAWORDER
			svl.lfree = pv->next;
			if(!svl.lfree) svl.lfst = NULL;
			pv->next = pl->head;
			pl->head = pv;
			if(!pv->next) pl->tail = pv;
#else
			svl.lfree = pv->next;
			if(!svl.lfree) svl.lfst = NULL;
			pv->next = NULL;
			if(pl->tail) pl->tail->next = pv;
			else pl->head = pv;
			pl->tail = pv;
#endif
			checkVertices(p);
		}
#if 0
		if(pl->flags & TEP_DRUNK){
			pv->pos[0] = pl->pos[0] + drseq(&rs) - .5;
			pv->pos[1] = pl->pos[1] + drseq(&rs) - .5;
			pv->pos[2] = pl->pos[2] + drseq(&rs) - .5;
		}
		else if(pl->flags & TEP_WIND){
			pv->x = (int)pl->x + rand() % 4 - 2;
			pv->y = (int)pl->y + rand() % 4 - 2;
		}
		else{
			pv->x = (int)pl->x;
			pv->y = (int)pl->y;
		}
#endif
		VECCPY(pv->pos, pl->pos);
		pv->dt = dt;
	//	pl->cnt++;
#ifndef NDEBUG
		p->debug.tevert_s++;
#endif
		}
		else{
#if DRAWORDER
			if(pl->head){
				VECCPY(pl->head->pos, pl->pos);
			}
#else
			if(pl->tail){
				VECCPY(pl->tail->pos, pl->pos);
			}
#endif
		}
		}
novertice:

		/* if a vertex is not added, let the next one carry the load. */
#if DRAWORDER
		if(!genvert && pl->head)
			pl->head->dt += dt;
#else
		if(!genvert && pl->tail)
			pl->tail->dt += dt;
#endif

		pl->life -= dt;
		/* if out, don't think of it anymore, with exceptions on return and reflect,
		  which is expceted to be always inside the region. */
		if(pl->life > 0){
			VECSADD(pl->velo, pl->grv, dt); /* apply gravity */
#if 0
			if(pl->flags & TEP_DRUNK){
				pl->vx += dt * ((drand() - .5) * 9000 - (pl->vx / 3));
				pl->vy += dt * ((drand() - .5) * 9000 - (pl->vy / 3));
			}
#endif
			VECSADD(pl->pos, pl->velo, dt);
		}
#if DRAWORDER
		else if(pl->head)
			pl->head->dt += dt;
#else
		else if(pl->tail)
			pl->tail->dt += dt;
#endif

#if ENABLE_RETURN
		/* reappear from opposite edge. prior than reflect */
		if(pl->flags & TEL_RETURN){
			if(pl->x < 0)
				pl->x += XSIZ;
			else if(XSIZ < pl->x)
				pl->x -= XSIZ;
			if(pl->y < 0)
				pl->y += YSIZ;
			else if(YSIZ < pl->y)
				pl->y -= YSIZ;
		}
		else
#endif
		/* rough approximation of reflection. rigid body dynamics are required for more reality. */
		if(pl->flags & TEP3_REFLECT){
			/* floor reflection only! */
			if(pl->pos[1] + pl->velo[1] * dt < 0)
				pl->velo[1] *= -REFLACTION;
		}

		pl = pl2;
	}
	checkVertices(p);
#ifndef NDEBUG
	if(p->ml){
	unsigned int i = 0;
	tefpol_t *pl = p->lactv;
	while(pl) i++, pl = pl->next;
	p->debug.tefpol_c = i;
	}
	if(svl.m){
	unsigned int i = svl.m;
	tevert_t *pv = svl.lfree;
	while(pv) i--, pv = pv->next;
	p->debug.tevert_c = i;
	}
#endif
#ifndef NDEBUG
	}
	p->debug.animtefpol = TimeMeasLap(&tm);
#endif
}

#ifndef NDEBUG
const struct tent3d_fpol_debug *Tefpol3DDebug(const struct tent3d_fpol_list *tepl){
	return &tepl->debug;
}
#endif

void DrawTefpol3D(tent3d_fpol_list *p, const double view[3], const struct glcull *glc){
#ifndef NDEBUG
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
#endif
	tefpol_t *pl = p->lactv;
#if DIRECTDRAW
	BITMAP bm;
	if(!pl) goto retur;
	GetObject(pwg->hVBmp, sizeof bm, &bm);
#endif
	for(; pl; pl = pl->next) if(pl->head){
		double o[3];
		int c = pl->cnt;
#if INTERP_CS
		COLORREF ocol;
#endif
		tevert_t *pv = pl->head;
		double t = 0;
		int linestrip = 0, beams = 0; /* current drawing method */
#if ENABLE_THICK
		struct gldBeamsData bd;
#else
		const int thick = 0;
#endif
#if DRAWORDER
		VECCPY(o, pl->pos);
#else
		VECCPY(o, pl->head->pos);
#endif
#if !DRAWORDER
		for(pv = pl->head; pv; pv = pv->next) t += ABS(pv->dt);
#endif
#if INTERP_CS
		ocol = ColorSequence(pl->cs, t);
/*		ocol = SWAPRB(ocol);*/
#endif
/*		int i;
		for(i = 0; i < 2; i++){
			PutPointWin(pwg, ox + rand() % 10 - 5, oy + rand() % 10 - 5, ColorSequence(pl->cs, 0));
		}*/
#if WINTEFPOL
		MoveToEx(pwg->hVDC, ox, oy, NULL);
#endif
/*		glBegin(GL_LINE_STRIP);*/
		gldColor32(ocol/*ColorSequence(pl->cs, t)*/);
/*		glVertex3dv(o);*/
		for(pv = pl->head; pv; c--){
			float dt;
#if INTERP_CS
			COLORREF ncol;
#endif
#if ENABLE_FINE
			int x, y, drawn;
			for(drawn = 0; !drawn || (pl->flags & TEP_FINE && drawn == 1); drawn++){
			if(pl->flags & TEP_FINE){
				x = drawn ? pv->x : (ox + pv->x) / 2;
				y = drawn ? pv->y : (oy + pv->y) / 2;
			}
			else
				x = pv->x, y = pv->y;
			if(pl->flags & TEP_SHAKE){
				x += rand() % 8 - 4;
				y += rand() % 8 - 4;
			}
#else
			const double (*d)[3] = &pv->pos;
/*			const int x = pl->flags & TEP_SHAKE ? pv->x + rand() % 6 - 3 : pv->x,
				y = pl->flags & TEP_SHAKE ? pv->y + rand() % 6 - 3 : pv->y;*/
#endif

#if ENABLE_FINE
			dt = pl->flags & TEP_FINE ? ABS(pv->dt) / 2 : ABS(pv->dt);
#else
			dt = ABS(pv->dt);
#endif

#if INTERP_CS
#	if DRAWORDER
			ncol = ColorSequence(pl->cs, t + dt);
#	else
			ncol = ColorSequence(pl->cs, t);
#	endif
/*			ncol = SWAPRB(ncol);*/
#endif

#if ENABLE_FORMS
			switch(pl->flags & TEL_FORMS){
			case 0:
#endif


				if(pv->dt < 0.){
					if(linestrip){
						linestrip = 0;
						glEnd();
					}
					beams = 0;
				}
				else
				{
#	if ENABLE_THICK
				{
				tent3d_flags_t thickness = pl->flags & TEP3_THICKNESS;
				double apparence;
				static const double visdists[5] = {.25 * .25, .5 * .5, 1.5 * 1.5, .5 * .5, .5 * .5};
				static const double thicknesses[5] = {.00, .001, .002, .005, .00025};
				if(thickness == TEP3_FAINT){
					struct random_sequence rs;
					init_rseq(&rs, (unsigned long)pv/*(unsigned long)((*d)[0] * (*d)[1] * 1e6)*/);
					ncol = COLOR32RGBA(COLOR32R(ncol),COLOR32G(ncol),COLOR32B(ncol),COLOR32A(ncol) * (rseq(&rs) % 256) / 256);
				}
				else if(thickness == TEP3_THICK && pv == pl->tail){
					ncol = COLOR32RGBA(COLOR32R(ncol),COLOR32G(ncol),COLOR32B(ncol),0);
				}
				apparence = !thickness || !glc ? 1. : fabs(glcullScale(&pv->pos, glc)) * thicknesses[thickness >> THICK_BIT];
				if(thickness && 1. < apparence/*VECSDIST(pv->pos, view) < visdists[thickness >> THICK_BIT]*/){
					double width =
						thickness == TEP3_THICK ? /*t < .5 ? t * .002 :*/ .001 :
						thickness == TEP3_THICKER ? t < .5 ? t * .004 : .002 : 
						thickness == TEP3_FAINT ? t < .5 ? t * .001 : .0005 : .005;
					if(linestrip){
						linestrip = 0;
						glEnd();
					}
					if(!beams){
						beams = 1;
						bd.cc = 0;
						bd.solid = 0;
						gldBeams(&bd, view, o, width, ncol);
					}
					gldColor32(ocol);
/*					gldGradBeam(view, o, pv->pos, .001, ncol);*/
					gldBeams(&bd, view, *d, width, ncol);
/*					glBegin(GL_LINES);
					glColor3ub(0,0,255);
					glVertex3dv(o);
					glVertex3dv(*d);
					glEnd();*/
				}
				else{
					if(beams){
						beams = 0;
					}
					if(0. < apparence){
						if(!linestrip){
							linestrip = 1;
							glBegin(GL_LINE_STRIP);
							glColor4ub(COLOR32R(ocol), COLOR32G(ocol), COLOR32B(ocol), COLOR32A(ocol) * apparence);
							/*ColorSequence(pl->cs, t + pv->dt)*/
							glVertex3dv(o);
						}
#	endif
						glColor4ub(COLOR32R(ncol), COLOR32G(ncol), COLOR32B(ncol), COLOR32A(ncol) * apparence);
						/*gldColor32(ColorSequence(pl->cs, t + pv->dt));*/
						glVertex3dv(*d);
#	if ENABLE_THICK
					}
				}
				}
#	endif
				}

#if ENABLE_FORMS
				break;
			case TEL_POINT:
				PutPointWin(pwg, pv->x, pv->y, ColorSequence(pl->cs, t));
				break;
			case TEL_CIRCLE:
				CircleWin(pwg, pv->x, pv->y, (ABS(ox - pv->x) + ABS(oy - pv->y)) / 2, ColorSequence(pl->cs, t));
				break;
			case TEL_FILLCIRCLE:
				FillCircleWin(pwg, pv->x, pv->y, (ABS(ox - pv->x) + ABS(oy - pv->y)) / 2, ColorSequence(pl->cs, t));
				break;
			case TEL_RECTANGLE:{
				int rad = (ABS(ox - pv->x) + ABS(oy - pv->y)) / 2;
				RectangleWin(pwg, pv->x - rad, pv->y - rad, pv->x + rad, pv->y + rad, ColorSequence(pl->cs, t));
				break;}
			case TEL_FILLRECT:{
				int rad = (ABS(ox - pv->x) + ABS(oy - pv->y)) / 2;
				FillRectangleWin(pwg, pv->x - rad, pv->y - rad, pv->x + rad, pv->y + rad, ColorSequence(pl->cs, t));
				break;}
			case TEL_PENTAGRAPH:{
				double rad = (ABS(ox - pv->x) + ABS(oy - pv->y)) / 2, ang = 9/*drand() * 2 * M_PI*/;
				int i;
				HPEN hPen;
				hPen = CreatePen(PS_SOLID, 0, ColorSequence(pl->cs, t));
				if(!hPen) break;
				SelectObject(pwg->hVDC, hPen);
				MoveToEx(pwg->hVDC, pv->x + cos(ang) * rad, pv->y + sin(ang) * rad, NULL);
				for(i = 0 ; i < 5; i++){
					ang += 2 * M_PI * 2 / 5.;
					LineTo(pwg->hVDC, pv->x + cos(ang) * rad, pv->y + sin(ang) * rad);
				}
				DeleteObject(hPen);
				break;}
			case TEL_HEXAGRAPH:{
				HFONT hFont;
				char *s;
				hFont = CreateFont(MIN((ABS(ox - pv->x) + ABS(oy - pv->y)), 20), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL);
				if(!hFont) break;
				SelectObject(pwg->hVDC, hFont);
				SetTextColor(pwg->hVDC, ColorSequence(pl->cs, t));
				switch(c % 7){
					case 6:
					case 0: s = "”š"; break;
					case 1: s = "—ó"; break;
					case 2: s = "â"; break;
					case 3: s = "–Å"; break;
					case 4: s = "‰ó"; break;
					case 5: s = "Š…"; break;
				}
				TextOut(pwg->hVDC, pv->x, pv->y, s, 2);
				DeleteObject(hFont);
				break;}
			default: assert(0);
			}
#endif
#if ENABLE_FINE
			if(pl->flags & TEP_FINE)
#if DRAWORDER
			t += dt / 2;
#else
			t -= dt / 2;
#endif
			else
#endif
#if DRAWORDER
			t += dt;
#else
			t -= dt;
#endif
#if ENABLE_FINE
			if(pl->flags & TEP_FINE && !drawn)
				ox = (ox + pv->x) / 2, oy = (oy + pv->y) / 2;
			}
#endif
#if INTERP_CS
			ocol = ncol;
#endif
			VECCPY(o, *d);
			pv = pv->next;
		}
		if(linestrip)
			glEnd();
#if ENABLE_HEAD
		if(pl->flags & TEP_HEAD){
#if LANG==JP
			HFONT hFont;
			hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL);
			if(!hFont) break;
			SelectObject(pwg->hVDC, hFont);
			SetTextColor(pwg->hVDC, ColorSequence(pl->cs, 0));
			TextOut(pwg->hVDC, pl->x - 10, pl->y - 10, s_electric[(int)pl%numof(s_electric)], 2);
			DeleteObject(hFont);
#else
		FillCircleWin(pwg, pl->x, pl->y, 5, ColorSequence(pl->cs, 0));
#endif
		}
#endif
	}
retur:;
#ifndef NDEBUG
	}
	p->debug.drawtefpol = TimeMeasLap(&tm);
#endif
}
