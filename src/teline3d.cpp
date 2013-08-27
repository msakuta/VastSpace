/** \file
 * \brief Temporary Entity LINE implementation.
 */
#include "tent3d.h"
#include "tent3d_p.h"
#include <cpplib/mat4.h>
#include <cpplib/quat.h>
extern "C"{
#include <clib/c.h>
#include <clib/suf/suf.h>
#include <clib/timemeas.h>
#include <clib/gl/gldraw.h>
}
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <new>
#include <algorithm>
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

#define COLISTRGB(c) COLOR32R(c), COLOR32G(c), COLOR32B(c)
#define COLIST(c) COLOR32R(c), COLOR32G(c), COLOR32B(c), COLOR32A(c)


namespace tent3d{

/// \brief Telines with customizable colors.
struct ColorTeline3 : Teline3{
	COLOR32 r;
	ColorTeline3(Teline3ConstructInfo &ci, COLOR32 color) : Teline3(ci), r(color){}
	COLOR32 getColor()const{
		return life < FADE_START ? (r & 0x00ffffff) | COLOR32RGBA(0,0,0,(GLubyte)(COLOR32A(r) * life / FADE_START)) : r;
	}
	double getLength()const;
	void transform(const tent3d_line_drawdata &dd)const;
};

/// \brief Line shaped, um, Teline.
struct LineTeline3 : ColorTeline3{
	LineTeline3(Teline3ConstructInfo &ci, COLOR32 color) : ColorTeline3(ci, color){}
	void draw(const tent3d_line_drawdata &)const override;
};

struct ExpandColorTeline3 : ColorTeline3{
	double maxlife;
	ExpandColorTeline3(Teline3ConstructInfo &ci, COLOR32 color) : ColorTeline3(ci, color), maxlife(ci.life){}
};

/// \brief Expanding cylinder over time.
struct CylinderTeline3 : ExpandColorTeline3{
	CylinderTeline3(Teline3ConstructInfo &ci, COLOR32 color) : ExpandColorTeline3(ci, color){}
	void draw(const tent3d_line_drawdata &)const override;
};

/// \brief A disc effect without motion.
///
/// It gradually disappears on its end of life.
///
/// I've named the macro TEL3_STATICDISK, but it's actually disc.
/// 'Disk' is shorthand of diskette, which is not originally related to circular shape,
/// but I cannot fix the macro name at once.
struct StaticDiscTeline3 : ExpandColorTeline3{
	StaticDiscTeline3(Teline3ConstructInfo &ci, COLOR32 color) : ExpandColorTeline3(ci, color){}
	void draw(const tent3d_line_drawdata &)const override;
protected:
	virtual double getRadius()const{return len;}
	virtual void getEndColors(COLOR32 &centerColor, COLOR32 &edgeColor)const{
		centerColor = getColor();
		edgeColor = getColor() & 0x00ffffff;
	}
};

/// \brief Expanding disk over time.
struct ExpanDiscTeline3 : StaticDiscTeline3{
	ExpanDiscTeline3(Teline3ConstructInfo &ci, COLOR32 color) : StaticDiscTeline3(ci, color){}
protected:
	double getRadius()const override{return (maxlife - life) / maxlife * getLength();}
	virtual void getEndColors(COLOR32 &centerColor, COLOR32 &edgeColor)const override{
		centerColor = getColor() & 0x00ffffff;
		edgeColor = getColor();
	}
};

/// \brief Kind of gas blob floating in space.
struct SpriteTeline3 : ColorTeline3{
	SpriteTeline3(Teline3ConstructInfo &ci, COLOR32 color) : ColorTeline3(ci, color){}
	void draw(const tent3d_line_drawdata &)const override;
};

/// \brief Teline with callback function.
struct CallbackTeline3 : Teline3{
	typedef void (*CallbackProc)(const Teline3CallbackData *, const struct tent3d_line_drawdata*, void*);
	CallbackProc callback;
	void *callback_hint;
	CallbackTeline3(Teline3ConstructInfo &ci, CallbackProc f, void *p) : Teline3(ci), callback(f), callback_hint(p){}
	void draw(const tent3d_line_drawdata &)const override;
};

/// \brief Container object for any Teline derived class objects.
///
/// It provides set of fixed size buffers, in which Teline objects are allocated by placement new syntax.
struct Teline3Node{
	/// We'd like to use std::max template instead of MAX macro, but this value is static, which means no
	/// runtime evaluation is possible.
	/// Note that no matter how large we reserve the buffer, there's always the possibility of user defined
	/// Teline descendants which exceeds the size.  We raise an exception at runtime in that case.
	/// Or should we support such cases by using additional heap memory?
	static const size_t size = MAX(sizeof(LineTeline3), MAX(sizeof(CylinderTeline3), MAX(sizeof(ExpanDiscTeline3), sizeof(CallbackTeline3))));
	Teline3Node *next;
	char buf[size];
};

Teline3::Teline3(const Teline3ConstructInfo &ci) : Teline3ConstructInfo(ci)
{
	/* filter these bits a little */
	if(omg != vec3_000){
		tent3d_flags_t x, y, z;
		x = (omg ? omg[0] : 0) ? TEL3_ROTATEX : 0;
		y = (omg ? omg[1] : 0) ? TEL3_ROTATEY : 0;
		z = (omg ? omg[2] : 0) ? TEL3_ROTATEZ : 0;
		flags &= ~TEL3_ROTATE; /* truncate any given flags */
		flags |= x | y | z;
	}
}

/// \brief The virtual destructor for Teline3.
Teline3::~Teline3(){}

struct tent3d_line_extra_list; // forward declaration

/** \brief List of Telines with custom allocator.
 *
 * This is a pointer list pool, which does not use heap allocation or shifting
 * during the simulation which can be serious overhead.
 */
struct Teline3List{
	unsigned m; /* cap for total allocations of elements */
	unsigned ml; /* allocated elements in l */
	unsigned mex; /* allocated elements in an extra list */
	Teline3Node *l;
	Teline3Node *lfree, *lactv, *last;
	struct tent3d_line_extra_list *ex; /* extra space is allocated when entities hits their max count */
	unsigned bs; /* verbose buffer size */
#ifndef NPROFILE
	struct tent3d_line_debug debug;
#endif
};

struct tent3d_line_extra_list{
	struct tent3d_line_extra_list *next;
	Teline3Node l[1]; /* variable */
};


/* allocator and constructor */
Teline3List *NewTeline3D(unsigned maxt, unsigned init, unsigned unit){
	/* it's silly that max is defined as a macro */
	int i;
	Teline3List *ret;
	ret = new Teline3List;
	if(!ret) return NULL;
	ret->lfree = ret->l = new Teline3Node[init];
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
	ret->debug.so_teline3_t = sizeof(Teline3);
	ret->debug.teline_m = ret->bs;
	ret->debug.teline_c = 0;
	ret->debug.teline_s = 0;
#endif
	return ret;
}

/*
Teline3List *NewTeline3DFunc(unsigned maxt, unsigned init, unsigned unit, warf_t *w){
	Teline3List *ret;
	ret = NewTeline3D(maxt, init, unit);
	ret->w = w;
	return ret;
}
*/
/* destructor and deallocator */
void DelTeline3D(Teline3List *p){
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

static Teline3Node *alloc_teline(Teline3List *p)
{
	Teline3Node *pl;
	if(!p || !p->m) return NULL;
	pl = p->lfree;
	if(!pl){
		if(p->mex && p->bs + p->mex <= p->m){
			struct tent3d_line_extra_list *ex;
			unsigned i;
			ex = (tent3d_line_extra_list*)malloc(offsetof(struct tent3d_line_extra_list, l) + p->mex * sizeof *ex->l); /* struct hack alloc */
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

#ifndef NDEBUG
	p->debug.teline_s++;
#endif
	return pl;
}

void AddTeline3D(Teline3List *p, const Vec3d &pos, const Vec3d &velo,
	double len, const Quatd &rot, const Vec3d &omg, const Vec3d &grv,
	COLOR32 col, tent3d_flags_t flags, float life)
{
	Teline3Node *pl = alloc_teline(p);
	if(!pl) return;
	Teline3ConstructInfo ci = {pos, velo, len, rot, omg, life, grv, flags};
	switch(flags & TEL_FORMS){
		case TEL3_CYLINDER: new(pl->buf) CylinderTeline3(ci, col); break;
		case TEL3_STATICDISK: new(pl->buf) StaticDiscTeline3(ci, col); break;
		case TEL3_EXPANDISK: new(pl->buf) ExpanDiscTeline3(ci, col); break;
		case TEL3_EXPANDGLOW:
		case TEL3_SPRITE: new(pl->buf) SpriteTeline3(ci, col); break;
		case TEL3_EXPANDTORUS:
		default: new(pl->buf) LineTeline3(ci, col);
	}
}

void AddTelineCallback3D(Teline3List *p, const Vec3d &pos, const Vec3d &velo,
	double len, const Quatd &rot, const Vec3d &omg, const Vec3d &grv,
	void (*f)(const Teline3CallbackData*, const struct tent3d_line_drawdata*, void*), void *pd, tent3d_flags_t flags, float life)
{
	flags &= ~TEL_FORMS; // Clear explicitly set form bits to make "callback" form take effect.
	Teline3Node *pl = alloc_teline(p);
	if(!pl) return;
	Teline3ConstructInfo ci = {pos, velo, len, rot, omg, life, grv, flags |= TEL3_CALLBACK | TEL3_NOLINE};
	new(pl->buf) CallbackTeline3(ci, f, pd);
}


bool Teline3::update(double dt){
	life -= dt;
	if(life < 0)
		return false;
/*		if(pl->flags & TEL3_ACCELFUNC && p->w && p->w->vft->accel){
			avec3_t accel;
			p->w->vft->accel(p->w, &accel, &pl->pos, &pl->velo);
			VECSADD(pl->velo, accel, dt);
		}*/
	velo += grv * dt; /* apply gravity */
	pos += velo * dt;

	// Really should think of anisotropic moment of inertia
	if(!(flags & TEL3_HEADFORWARD) && omg != vec3_000){
		rot = rot.quatrotquat(omg * dt);
	}

	return true;
}

/* animate every line */
void AnimTeline3D(Teline3List *p, double dt){
	Teline3Node *pl = p->lactv, **ppl = &p->lactv;
	while(pl){
		Teline3Node *pl2;
		Teline3 *teline = reinterpret_cast<Teline3*>(pl->buf);

		/* delete condition */
		if(!teline->update(dt)){

			// Explicit invocation of the virtual destructor
			teline->~Teline3();

			// Placement delete, actually does nothing. Provided just for symmetry.
			operator delete(pl->buf, teline);

			/* roll pointers */
			if(pl == p->last && *ppl != p->lactv)
				p->last = (Teline3Node*)((char*)ppl - offsetof(Teline3Node, next));
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
		pl = pl2;
	}
#ifndef NDEBUG
	if(p->ml){
		unsigned int i = 0;
		Teline3Node *pl = p->lactv;
		while(pl) i++, pl = pl->next;
		p->debug.teline_c = i;
	}
#endif
}

/* since 3d graphics have far more parameters than that of 2d, we pack those variables
  to single structure to improve performance of function calls. */
void DrawTeline3D(Teline3List *p, struct tent3d_line_drawdata *dd){
	timemeas_t tm;
	TimeMeasStart(&tm);
	if(!p)
		return;
	{
//	double br, lenb;
	Teline3Node *pl = p->lactv;

	while(pl){
#if 1
		Teline3 *line = reinterpret_cast<Teline3*>(pl->buf);
		line->draw(*dd);
#else
		tent3d_flags_t form = pl->flags & TEL_FORMS;
		Quatd fore;
		double *rot = pl->flags & TEL3_HEADFORWARD ? fore : pl->rot;
//		COLOR32 col;
		br = MIN(pl->life / FADE_START, 1.0);

		if(pl->flags & TEL3_HEADFORWARD){
			Vec3d dr = pl->velo.norm();

			Quatd q = Quatd::direction(dr);
			fore = q * pl->rot;
		}

		if(form == TEL3_EXPANDTORUS){
			int i;
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

			if(form == TEL3_PSEUDOSPHERE){
				GLubyte col[4];
				COLOR32UNPACK(col, pl->mdl.r);
				col[3] = pl->life < FADE_START ? GLubyte(COLOR32A(pl->mdl.r) * pl->life / FADE_START) : COLOR32A(pl->mdl.r);
				gldPseudoSphere(pl->pos, lenb, col);
			}
			else if(form == TEL3_CYLINDER){
			}
			else if(form == TEL3_EXPANDISK || form == TEL3_STATICDISK){
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
			}
			else{ /* if(forms == TEL_POINT) */
				glBegin(GL_POINTS);
				glVertex3dv(pl->pos);
				glEnd();
			}
			glPopMatrix();
		}
#endif


		pl = pl->next;
	}
	}
#ifndef NPROFILE
	p->debug.drawteline = TimeMeasLap(&tm);
#endif
}

double ColorTeline3::getLength()const{
	double lenb;

	/* shrink over its lifetime */
	if(flags & TEL_SHRINK && life < SHRINK_START)
		lenb = len / SHRINK_START * life;
	else
		lenb = len;

	/* recalc length by its velocity. */
	if(flags & TEL_VELOLEN)
		lenb *= velo.len();

	return lenb;
}

void ColorTeline3::transform(const tent3d_line_drawdata &dd)const{
	if(flags & TEL3_NEAR){
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
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	if(flags & TEL3_INVROTATE)
		glMultMatrixd(dd.invrot);
}

void Teline3::draw(const tent3d_line_drawdata &dd)const{}

void LineTeline3::draw(const tent3d_line_drawdata &dd)const{
	double lenb = getLength();

	COLOR32 col = getColor();

	Vec3d start, end;
	double width = flags & TEL3_HEADFORWARD ? .002 : lenb * .2;
	if(flags & TEL3_HEADFORWARD){
		lenb /= velo.len();
		start = pos - velo * lenb;
		end = pos + velo * lenb;
	}
	else{
		Mat4d mat = Quatd(rot).tomat4();
		start = mat.dvp3(vec3_001);
		start *= lenb;
		end = -start;
		start += pos;
		end += pos;
	}

	if(flags & TEL3_THICK && (dd.viewpoint - pos).slen() < /*width * width * 1e-3 * 1e-3 /*/.5 * .5){
		if(flags & TEL3_FADEEND){
			if(flags & TEL3_FADEEND)
				glColor4ub(COLISTRGB(col), 0);
			else
				glColor4ub(COLIST(col));
			gldGradBeam(dd.viewpoint, start, end, width, col);
		}
		else
			gldBeam(dd.viewpoint, start, end, width);
	}
	else{
		glBegin(GL_LINES);
		if(flags & TEL3_FADEEND)
			glColor4ub(COLISTRGB(col), 0);
		else
			glColor4ub(COLIST(col));
		glVertex3dv(start);
		if(flags & TEL3_FADEEND)
			glColor4ub(COLIST(col));
		glVertex3dv(end);
		glEnd();
	}
}

void CylinderTeline3::draw(const tent3d_line_drawdata &dd)const{
	COLOR32 col = getColor();
	double lenb = getLength();
	GLubyte cc[4], ce[4];
	double (*cuts)[2], radius;
	ce[0] = cc[0] = COLOR32R(col);
	ce[1] = cc[1] = COLOR32G(col);
	ce[2] = cc[2] = COLOR32B(col);
	cc[3] = COLOR32A(col);
	ce[3] = 0;
	cuts = CircleCuts(5);
	radius = (maxlife - life) * lenb;
	glPushMatrix();
	transform(dd);
	glBegin(GL_QUAD_STRIP);
	for(int i = 0; i <= 5; i++){
		int k = i % 5;
		glColor4ubv(cc);
		glVertex3d(cuts[k][0] * radius, cuts[k][2] * radius, 0.);
		glColor4ubv(ce);
		glVertex3d(cuts[k][0] * radius, cuts[k][2] * radius, radius);
	}
	glEnd();
	glPopMatrix();
}

void StaticDiscTeline3::draw(const tent3d_line_drawdata &dd)const{
	COLOR32 cc, ce; /* center color, edge color */
	getEndColors(cc, ce);
	double radius = getRadius();
	double (*cuts)[2] = CircleCuts(16);
	glPushMatrix();
	transform(dd);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ub(COLIST(cc));
	glVertex3d(0., 0., 0.);
	glColor4ub(COLIST(ce));
	for(int i = 0; i <= 16; i++){
		int k = i % 16;
		glVertex3d(cuts[k][0] * radius, cuts[k][1] * radius, 0.);
	}
	glEnd();
	glPopMatrix();
}

void SpriteTeline3::draw(const tent3d_line_drawdata &dd)const{
	COLOR32 col = getColor();
	double lenb = getLength();
	double (*cuts)[2] = CircleCuts(10);
	glPushMatrix();
	transform(dd);
	glScaled(lenb, lenb, lenb);
	glBegin(GL_TRIANGLE_FAN);
	glColor4ub(COLIST(col));
	glVertex3d(0., 0., 0.);
	glColor4ub(COLISTRGB(col), 0);
	for(int i = 0; i <= 10; i++){
		int k = i % 10;
		glVertex3d(cuts[k][1], cuts[k][0], 0.);
	}
	glEnd();
	glPopMatrix();
}

void CallbackTeline3::draw(const tent3d_line_drawdata &dd)const{
	callback(this, &dd, callback_hint);
}
}

void *operator new(size_t size, Teline3List &a){
	assert(size <= Teline3Node::size);
	Teline3Node *ret = alloc_teline(&a);
	if(ret)
		return ret->buf;
	else throw std::bad_alloc();
}


#ifndef NPROFILE
const tent3d_line_debug *Teline3DDebug(const Teline3List *p){
	return &p->debug;
}
#endif
