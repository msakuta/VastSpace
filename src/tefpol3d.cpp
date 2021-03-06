/** \file
 *\brief Implementation of Tefpol, TefpolList and TefpolVertex classes.
 */
#include "tefpol3d.h"
#include "tent3d_p.h"
#include "draw/material.h"
#include "glstack.h"
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

namespace tent3d{

/*#############################################################################
	macro definitions
#############################################################################*/
#define DRAWORDER 0 /* whether draw from the head to the tail, otherwise reverse */
#define ENABLE_FINE 0 /* granularity control for line graduation */
#define ENABLE_ROUGH 0 /* less fine */
#define ENABLE_THICK 1 /* thickness control in conjunction with WinAPI drawing */
#define ENABLE_HEAD 0 /* head sprite */
#define INTERP_CS 1 /* Liner interpolation rather than calling ColorSequence() everytime */
#define VERTEX_SIZE 0x8000

#define TEP3_THICKNESS (TEP3_THICK|(TEP3_THICK<<1)|(TEP3_THICK<<2))
#define THICK_BIT 7
#define TEP_BLENDMASK (TEP_SOLIDBLEND|(TEP_SOLIDBLEND<<1))
#define TEP3_MOVABLE (Tefpol::Movable) /* cannot be freed unless explicitly specified by the caller */
//const tefpol_flags_t TEP3_MOVABLE = (1<<16);
#define TEP3_SKIP    (Tefpol::Skip) /* skip generation of vertex for movable tefpol */
//const tefpol_flags_t TEP3_SKIP = (1<<17);

#ifndef ABS
#define ABS(a) (0<(a)?(a):-(a))
#endif

/*=============================================================================
	type definitions
=============================================================================*/
typedef struct color_sequence colseq_t;

/* global vertice pool, used by multiple entities */
static struct te_vertice_list{
	unsigned m;
	TefpolVertex *l;
	TefpolVertex *lfree, *lfst;
} svl = {0, NULL};

typedef struct TefpolList tepl_t;




/*-----------------------------------------------------------------------------
	function definitions
-----------------------------------------------------------------------------*/

#if 1
#define checkVertices(a)
#else
static void checkVertices(tepl_t *p){
	Tefpol *pl = p->lactv;
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

void TefpolList::freeTefpolExtra(struct te_fpol_extra_list *p){
	if(p->next) freeTefpolExtra(p->next);
	free(p);
}

TefpolList::TefpolList(unsigned maxt, unsigned init, unsigned unit){
	int i;
	TefpolList *ret = this;
	ret->lfree = ret->l = (Tefpol*)malloc(init * sizeof *ret->l);
	if(!ret->l) throw NULL;
	ret->m = maxt;
	ret->ml = ret->bs = init;
	ret->mex = unit;
	ret->lactv = ret->last = NULL;
	ret->ex = NULL;
	for(i = 0; i < (int)(init - 1); i++)
		ret->l[i].next = &ret->l[i+1];
	if(init) ret->l[init-1].next = NULL;
#ifndef NDEBUG
	ret->debug.so_tefpol3_t = sizeof(Tefpol);
	ret->debug.so_tevert3_t = sizeof(tevert_t);
	ret->debug.tefpol_m = ret->bs;
	ret->debug.tefpol_c = 0;
	ret->debug.tefpol_s = 0;
	ret->debug.tevert_c = 0;
	ret->debug.tevert_s = 0;
#endif
}

/* destructor and deallocator */
void DelTefpol(tepl_t *p){
	delete p;
}

TefpolList::~TefpolList(){
	if(this->l){
		Tefpol *pl = this->lactv;
		while(pl){
			freeVertex(&pl->head, pl->tail);
			pl = pl->next;
		}
		free(this->l);
	}
	if(this->ex) freeTefpolExtra(this->ex);
}

Tefpol *TefpolList::allocTefpol3D(const Vec3d &pos, const Vec3d &velo,
			   const Vec3d &grv, const colseq_t *col, tefpol_flags_t f, double life)
{
	TefpolList *const p = this;
	Tefpol *pl;
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
			Tefpol **ppl = &p->lactv; /* reuse the oldest active node */
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

	pl->pos = pos;
	pl->velo = velo;
	pl->grv = grv;
	pl->cs = col;
	pl->flags = f;
	pl->cnt = 0;
	pl->life = life;
	pl->texture = 0;
	pl->head = pl->tail = NULL;

#ifndef NDEBUG
	p->debug.tefpol_s++;
#endif
	return pl;
}

void Tefpol::move(const Vec3d &pos, const Vec3d &velo, const double life, int skip){
	Tefpol *pl = this;
	if(!pl || !(pl->flags & TEP3_MOVABLE))
		return;
	pl->pos = pos;
	if(velo)
		pl->velo = velo;
	else
		pl->velo.clear();
	pl->life = life;
	if(skip)
		pl->flags |= TEP3_SKIP;
	else
		pl->flags &= ~TEP3_SKIP;
}

void Tefpol::immobilize(){
	flags &= ~TEP3_MOVABLE;
}

void TefpolList::anim(double dt){
	TefpolList *p = this;
#ifndef NDEBUG
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
#endif
	Tefpol *pl = p->lactv, **ppl = &p->lactv;

	while(pl){
		Tefpol *pl2;
#	if ENABLE_THICK
		tefpol_flags_t thickness = pl->flags & TEP3_THICKNESS;
		double width =
			thickness == TEP3_THICK ? 1. :
			thickness == TEP3_THICKER ? 2. :
			thickness == TEP3_FAINT ? 0.5 : 5.;
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
				p->last = (Tefpol*)((byte*)ppl - offsetof(Tefpol, next));
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
				pl->head->pos = pl->pos;
				pl->head->dt = -ABS(pl->head->dt) - dt;
			}
#else
			if(pl->tail){
				pl->tail->pos = pl->pos;
				pl->tail->dt = -ABS(pl->tail->dt) - dt;
			}
#endif
		}
		else{ // If not skipping, try to add vertices.
			bool genvert = pl->life > 0 && (!pl->head || !pl->tail || (thickness ? width * width < (pl->pos - pl->tail->pos).slen() : pl->pos != pl->head->pos));
			if(genvert){
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
					pv->pos = pl->pos;
					pv->dt = dt;
				//	pl->cnt++;
#ifndef NDEBUG
					p->debug.tevert_s++;
#endif
				}
				else{
#if DRAWORDER
					if(pl->head){
						pl->head->pos = pl->pos;
					}
#else
					if(pl->tail){
						pl->tail->pos = pl->pos;
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
		}

		pl->life -= dt;
		/* if out, don't think of it anymore, with exceptions on return and reflect,
		  which is expceted to be always inside the region. */
		if(pl->life > 0){
			pl->velo += pl->grv * dt; /* apply gravity */
#if 0
			if(pl->flags & TEP_DRUNK){
				pl->vx += dt * ((drand() - .5) * 9000 - (pl->vx / 3));
				pl->vy += dt * ((drand() - .5) * 9000 - (pl->vy / 3));
			}
#endif
			pl->pos += pl->velo * dt;
		}
#if DRAWORDER
		else if(pl->head)
			pl->head->dt += dt;
#else
		else if(pl->tail)
			pl->tail->dt += dt;
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
	Tefpol *pl = p->lactv;
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


void DrawTefpol3D(TefpolList *p, const Vec3d &view, const struct glcull *glc){
	p->draw(view, glc);
}

void TefpolList::draw(const Vec3d &view, const glcull *glc){
	TefpolList *const p = this;
#ifndef NDEBUG
	timemeas_t tm;
	TimeMeasStart(&tm);
	{
#endif
	Tefpol *pl = p->lactv;

	for(; pl; pl = pl->next) if(pl->head){

		GLattrib attrib(GL_TEXTURE_BIT);
		if(pl->texture){
			glCallList(pl->texture);
		}

		Vec3d o;
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
		o = pl->pos;
#else
		o = pl->head->pos;
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
			const Vec3d &d = pv->pos;
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
				tefpol_flags_t thickness = pl->flags & TEP3_THICKNESS;
				double apparence;
				static const double visdists[5] = {250. * 250., 500. * 500., 1500. * 1500., 500. * 500., 500. * 500.};
				static const double thicknesses[5] = {0.0, 1., 2., 5., 0.25};
				if(thickness == TEP3_FAINT){
					struct random_sequence rs;
					init_rseq(&rs, (unsigned long)pv/*(unsigned long)((*d)[0] * (*d)[1] * 1e6)*/);
					ncol = COLOR32RGBA(COLOR32R(ncol),COLOR32G(ncol),COLOR32B(ncol),COLOR32A(ncol) * (rseq(&rs) % 256) / 256);
				}
				else if(thickness == TEP3_THICK && pv == pl->tail){
					ncol = COLOR32RGBA(COLOR32R(ncol),COLOR32G(ncol),COLOR32B(ncol),0);
				}
				apparence = !thickness || !glc ? 1. : fabs(glcullScale(~pv->pos, glc)) * thicknesses[thickness >> THICK_BIT];
				if(thickness && 1. < apparence/*VECSDIST(pv->pos, view) < visdists[thickness >> THICK_BIT]*/){
					double width =
						thickness == TEP3_THICK ? /*t < .5 ? t * 2. :*/ 1. :
						thickness == TEP3_THICKER ? t < .5 ? t * 4. : 2. : 
						thickness == TEP3_FAINT ? t < .5 ? t * 1. : 0.5 : 5.;
					if(linestrip){
						linestrip = 0;
						glEnd();
					}
					if(!beams){
						beams = 1;
						bd.cc = 0;
						bd.solid = 0;
						bd.tex2d = 1;
						gldBeams(&bd, view, o, width, ncol);
					}
					gldColor32(ocol);
/*					gldGradBeam(view, o, pv->pos, .001, ncol);*/
					gldBeams(&bd, view, d, width, ncol);
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
						glVertex3dv(d);
#	if ENABLE_THICK
					}
				}
				}
#	endif
				}

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
			o = d;
			pv = pv->next;
		}
		if(linestrip)
			glEnd();
	}
#ifndef NDEBUG
	}
	p->debug.drawtefpol = TimeMeasLap(&tm);
#endif
}

}
