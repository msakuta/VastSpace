#define _CRT_SECURE_NO_WARNINGS

#include "coordsys.h"
#include "astro.h"
#include "viewer.h"
#include "player.h"
#include "stellar_file.h"
extern "C"{
#include "calc/calc.h"
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/avec4.h>
#include <clib/timemeas.h>
}
#include <cpplib/gl/cullplus.h>
#include <stddef.h>
#include <assert.h>

#define numof(a) (sizeof(a)/sizeof*(a))

long tocs_invokes = 0;
long tocs_parent_invokes = 0;
long tocs_children_invokes = 0;

const char *CoordSys::classname()const{
	return "CoordSys";
}

CoordSys **CoordSys::legitimize_child(){
	if(parent){
		CoordSys **ppcs;
		for(ppcs = &parent->children; *ppcs; ppcs = &(*ppcs)->next) if(*ppcs == this)
			return ppcs; /* i was already legitimized! */

		/* append the child into family list */
		next = parent->children;
		parent->children = this;
		return &parent->children;
	}
	return NULL;
}

void CoordSys::adopt_child(CoordSys *newparent){
	CoordSys *adoptee = this;
	CoordSys **ppcs2;
	CoordSys dummycs, **ppdummycs;
	Vec3d pos, velo;
	Quatd q;

	/* already adopted */
	if(adoptee->parent == newparent)
		return;

	/* temporarily make a dummy coordinate system to attend the system tree. */
	dummycs = *adoptee;
	dummycs.parent = newparent;
	ppdummycs = dummycs.legitimize_child();

	/* obtain relative rotation matrix between old node to new node. */
	dummycs.qrot = quat_u;
	q = adoptee->tocsq(newparent);
/*	MAT4IDENTITY(dummycs.rot);
	MAT4IDENTITY(dummycs.irot);
	tocsim(mat, newparent, adoptee);*/

	/* calculate relativity */
	pos = newparent->tocs(adoptee->pos, adoptee->parent);
	velo = newparent->tocsv(adoptee->velo, adoptee->pos, adoptee->parent);

	/* remove from old family's list because one cannot be child of 2 or more
	  ancestory. this is one of the few points that is different from real
	  family tree; there's no marriage.
	   If the child is orphan, no settlement is needed. */
	if(adoptee->parent) for(ppcs2 = &adoptee->parent->children; *ppcs2; ppcs2 = &(*ppcs2)->next) if(*ppcs2 == adoptee){
		*ppcs2 = adoptee->next;
		break;
	}

	adoptee->pos = pos;
	adoptee->velo = velo;
	adoptee->omg = q.trans(adoptee->omg);
/*	mat4mp(dummycs.rot, adoptee->rot, mat);*/
	adoptee->qrot = q;
/*	MAT4CPY(adoptee->rot, mat);
	MAT4TRANSPOSE(adoptee->irot, adoptee->rot);*/

	/* unlink dummy coordinate system */
	*ppdummycs = dummycs.next;

	/* recognize the new parent */
	adoptee->parent = newparent;

	/* append the child into new family list */
	adoptee->next = newparent->children;
	newparent->children = adoptee;
}



static int findchild(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, const CoordSys *skipcs, int delta){
#if 1
	CoordSys *cs2;
	tocs_children_invokes++;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
#else
	int i;
	for(i = 0; i < cs->nchildren; i++) if(cs->children[i] && cs->children[i] != skipcs)
	{
		const coordsys *cs2 = cs->children[i];
#endif
		Vec3d v1, v;
		if(delta){
			v1 = src - cs2->velo;
			v = cs2->qrot.itrans(v1);
		}
		else{
			v1 = src - cs2->pos;
			v = cs2->qrot.itrans(v1);
		}
		if(retcs == cs2){
			ret = v;
			return 1;
		}
		if(findchild(ret, retcs, v, cs2, NULL, delta))
			return 1;
	}
	return 0;
}

static int findparent(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, int delta){
	Vec3d v1, v;
	tocs_parent_invokes++;

	if(!cs->parent)
		return 0;

	if(delta){
		v = cs->qrot.trans(src);
		v += cs->velo;
	}
	else{
		v1 = cs->qrot.trans(src);
		v = v1 + cs->pos;
	}

	if(cs->parent == retcs){
		ret = v;
		return 1;
	}

	/* do not scan subtrees already checked! */
	if(findchild(ret, retcs, v, cs->parent, cs, delta))
		return 1;
	return findparent(ret, retcs, v, cs->parent, delta);
}

Vec3d CoordSys::tocs(const Vec3d &src, const CoordSys *cs)const{
	Vec3d ret;
	tocs_invokes++;
	if(cs == this)
		ret = src;
	else if(findchild(ret, this, src, cs, NULL, 0))
		return ret;
	else if(!findparent(ret, this, src, cs, 0))
		ret = src;
	return ret;
}

static int findchildv(Vec3d ret, const CoordSys *retcs, const Vec3d src, const Vec3d srcpos, const CoordSys *cs, const CoordSys *skipcs){
#if 1
	CoordSys *cs2;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
#else
	int i;
	for(i = 0; i < cs->nchildren; i++) if(cs->children[i] && cs->children[i] != skipcs){
		const coordsys *cs2 = cs->children[i];
#endif
		Vec3d v1, v, vrot, pos;

		/* position */
		v1 = srcpos - cs2->pos;
		pos = cs2->qrot.itrans(v1);
/*		MAT4VP3(pos, cs2->irot, v1);*/

		/* velocity */
		v1 = src - cs2->velo;
		vrot = cs2->omg.vp(pos);
		v1 -= vrot;
		v = v1;
		v = cs2->qrot.itrans(v1);
/*		MAT4DVP3(v, cs2->irot, v1);*/

		if(retcs == cs2){
			ret = v;
			return 1;
		}
		if(findchildv(ret, retcs, v, pos, cs2, NULL))
			return 1;
	}
	return 0;
}

static int findparentv(Vec3d ret, const CoordSys *retcs, const Vec3d src, const Vec3d srcpos, const CoordSys *cs){
	Vec3d v1, v, vrot, pos;

	if(!cs->parent)
		return 0;

	/* velocity */
	v = cs->qrot.trans(src);
/*	MAT4DVP3(v, cs->rot, src);*/
	VECADDIN(v, cs->velo);
	VECVP(vrot, cs->omg, srcpos);
	v1 = cs->qrot.trans(vrot);
/*	MAT4DVP3(v1, cs->rot, vrot);*/
	VECADDIN(v, v1);
/*	VECADDIN(v1, vrot);*/
/*	VECADDIN(v, v1);*/
/*	VECADDIN(v, cs->velo);*/

	if(cs->parent == retcs){
		VECCPY(ret, v);
		return 1;
	}

	/* position */
	v1 = cs->qrot.trans(srcpos);
/*	MAT4VP3(v1, cs->rot, srcpos);*/
	VECADD(pos, v1, cs->pos);

	/* do not scan subtrees already checked! */
	if(findchildv(ret, retcs, v, pos, cs->parent, cs))
		return 1;
	return findparentv(ret, retcs, v, pos, cs->parent);
}

Vec3d CoordSys::tocsv(const Vec3d src, const Vec3d srcpos, const CoordSys *cs)const{
	Vec3d ret;
	if(cs == this)
		return src;
	else if(findchildv(ret, this, src, srcpos, cs, NULL))
		return ret;
	else if(!findparentv(ret, this, src, srcpos, cs))
		return src;
	return ret;
}





static int findchildq(Quatd &ret, const CoordSys *retcs, const Quatd &src, const CoordSys *cs, const CoordSys *skipcs){
#if 1
	CoordSys *cs2;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
#else
	int i;
	for(i = 0; i < cs->nchildren; i++) if(cs->children[i] && cs->children[i] != skipcs){
		coordsys *cs2 = cs->children[i];
#endif
		Quatd q;
		q = src * cs2->qrot;
		if(retcs == cs2){
			ret = q;
			return 1;
		}
		if(findchildq(ret, retcs, q, cs2, NULL))
			return 1;
	}
	return 0;
}

static int findparentq(Quatd &ret, const CoordSys *retcs, const Quatd &src, const CoordSys *cs){
	Quatd q, csirot;

	if(!cs->parent)
		return 0;

	q = src.imul(cs->qrot);

	if(cs->parent == retcs){
		QUATCPY(ret, q);
		return 1;
	}

	/* do not scan subtrees already checked! */
	if(findchildq(ret, retcs, q, cs->parent, cs))
		return 1;
	return findparentq(ret, retcs, q, cs->parent);
}

Quatd CoordSys::tocsq(const CoordSys *cs)const{
	Quatd ret;
	if(cs == this){
		return quat_u;
	}
	else if(findchildq(ret, this, quat_u, cs, NULL))
		return ret;
	else if(!findparentq(ret, this, quat_u, cs))
		return quat_u;
	return ret;
}







Mat4d CoordSys::tocsm(const CoordSys *cs)const{
	Quatd q;
	q = this->tocsq(cs);
	return q.tomat4();
}

Mat4d CoordSys::tocsim(const CoordSys *cs)const{
	Quatd q;
	q = cs->tocsq(this);
	return q.tomat4();
}




bool CoordSys::belongs(const Vec3d &pos, const CoordSys *pos_cs)const{
	return pos.slen() < parent->rad * parent->rad;
}

CoordSys *findchildb(Vec3d &ret, const Vec3d &src, const CoordSys *cs, const CoordSys *skipcs){
	CoordSys *cs2;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
		Vec3d v1, v;
		v1 = src - cs2->pos;
		v = cs2->qrot.itrans(v1);
/*		mat4vp3(v, cs2->irot, v1);*/

		/* radius is measured in parent coordinate system and if the position is not within it,
		  we do not belong to this one. */
		if(cs2->belongs(v, cs2)){
			CoordSys *csret;
			if(csret = findchildb(ret, v, cs2, NULL))
				return csret;
			VECCPY(ret, v);
			return cs2;
		}
	}
	return NULL;
}

static const CoordSys *findparentb(Vec3d &ret, const Vec3d &src, const CoordSys *cs){
	Vec3d v1, v;
	const CoordSys *cs2 = cs->parent;

	/* if it is root node, we belong to it no matter how its radius is */
	if(!cs->parent){
		ret = src;
		return cs;
	}

	v1 = cs->qrot.trans(src);
/*	MAT4VP3(v1, cs->rot, src);*/
	v = v1 + cs->pos;

	/* if the position vector exceeds sphere's radius, check for parents.
	  otherwise, check for all the children. */
	if(cs->parent->belongs(src, cs2)){
		CoordSys *csret;
		/* do not scan subtrees already checked! */
		if(csret = findchildb(ret, v, cs->parent, cs))
			return csret;

		/* if no children has the vector in its volume, it's for parent. */
		VECCPY(ret, v);
		return cs2;
	}
	return findparentb(ret, v, cs->parent);
}

const CoordSys *CoordSys::belongcs(Vec3d &ret, const Vec3d &src, const CoordSys *cs)const{
	CoordSys *csret;
	if(csret = findchildb(ret, src, cs, NULL))
		return csret;
	else if(cs->belongs(src, cs))
		return cs;
	else
		return findparentb(ret, src, cs);
}

/* oneself is by definition excluded from ancestor */
bool CoordSys::is_ancestor_of(const CoordSys *object)const{
	const CoordSys *cs;
	for(cs = object->parent; cs; cs = cs->parent) if(cs == this)
		return true;
	return false;
}

CoordSys *CoordSys::findcs(const char *name){
	CoordSys *cs, *ret;
	if(!strcmp(this->name, name))
		return this;
	for(cs = children; cs; cs = cs->next) if(ret = cs->findcs(name))
		return ret;
	return NULL;
}

CoordSys *CoordSys::findcspath(const char *path){
	CoordSys *cs;
	const char *p;
	if(!*path || !this)
		return this;
	p = strchr(path, '/');
	if(!p)
		p = &path[strlen(path)];
	if(!strncmp("..", path, p - path)){
		if(!*p)
			return this->parent;
		else
			return parent->findcspath(p+1);
	}
	if(p) for(cs = this->children; cs; cs = cs->next) if(strlen(cs->name) == p - path && !strncmp(cs->name, path, p - path)){
		if(!*p)
			return cs;
		else
			return cs->findcspath(p+1);
	}
	return NULL;
}

/* partial path */
CoordSys *findcsppath(CoordSys *root, const char *path, const char *pathend){
	CoordSys *cs;
	const char *p;
	if(!*path || 0 <= path - pathend)
		return root;
	p = strchr(path, '/');
	if(!p || pathend - p < 0)
		p = pathend;
	if(p) for(cs = root->children; cs; cs = cs->next) if(!strncmp(cs->name, path, p - path))
		return findcsppath(cs, p+1, pathend);
	return NULL;
}

static int pastrcmp_invokes = 0;

static double ao_getvwpos(Vec3d *ret, Astrobj *a){
	extern Player pl;
	if(a->vwvalid & CoordSys::VW_POS)
		*ret = a->vwpos;
	else{
		*ret = pl.cs->tocs(a->pos, a->parent);
		a->vwpos = *ret;
		a->vwvalid |= CoordSys::VW_POS;
	}
	if(a->vwvalid & CoordSys::VW_SDIST)
		return a->vwsdist;
	else{
		a->vwsdist = (*ret - pl.pos).slen();
		a->vwvalid |= CoordSys::VW_SDIST;
		return a->vwsdist;
	}
}

int pastrcmp(const Astrobj **a, const Astrobj **b){
	Vec3d ap, bp;
	double ad, bd;
	pastrcmp_invokes++;
	if(!*a) return 1;
	else if(!*b) return -1;
/*	tocs(bp, pl.cs, (*b)->pos, (*b)->cs);*/
	ad = ao_getvwpos(&bp, const_cast<Astrobj*>(*b)) - (*b)->rad * (*b)->rad;
	bd = ao_getvwpos(&ap, const_cast<Astrobj*>(*a)) - (*a)->rad * (*a)->rad;
	return ad /*- (*b)->rad * (*b)->rad*/
		< bd ? 1 : bd < ad ? -1 : 0 /*- (*a)->rad * (*a)->rad*/;
}


void CoordSys::draw(const Viewer *vw){
	Vec3d cspos;
	cspos = vw->cs->tocs(avec3_000, this);
/*	if(cs->flags & CS_EXTENT && vw->gc && glcullFrustum(cspos, cs->rad, vw->gc))
		return;*/
	if((CS_EXTENT | CS_ISOLATED) == (flags & (CS_EXTENT | CS_ISOLATED))){
		CoordSys *a;
		int i, n = naorder;
/*		if(naorder != n)
			aorder = (Astrobj**)realloc(aorder, (naorder = n) * sizeof *aorder);
		for(a = aolist, i = 0; a; a = (Astrobj*)a->next, i++)
			aorder[i] = a;*/
/*		{
			timemeas_t tm;
			TimeMeasStart(&tm);*/
			qsort(aorder, n, sizeof*aorder, reinterpret_cast<int(*)(const void*,const void*)>(pastrcmp));
/*			printf("sortcs %d %lg\n", n, TimeMeasLap(&tm));
		}*/
		for(i = n - 1; 0 <= i; i--) if(aorder[i]){
			a = aorder[i];
			a->draw(vw);
		}
	}
}

extern double get_timescale();
void CoordSys::anim(double dt){
	Vec3d omgdt;
/*	extern double get_timescale();
	dt *= get_timescale();*/
	omgdt += omg * dt;
	qrot = qrot.quatrotquat(omgdt);
}

void CoordSys::postframe(){}
void CoordSys::endframe(){
	vwvalid = 0;
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next)
		cs->endframe();
}

Quatd CoordSys::rotation(const Vec3d &pos, const Vec3d &pyr, const Quatd &srcq)const{
	Quatd ret;
	Vec3d omg;
	omg = vec3_100 * pyr[0];
	ret = srcq.quatrotquat(omg);
	omg = vec3_010 * pyr[1];
	ret = ret.quatrotquat(omg);
	omg = vec3_001 * pyr[2];
	ret = ret.quatrotquat(omg);
	return ret;
}



CoordSys::CoordSys(){
	init(NULL, NULL);
}
CoordSys::CoordSys(const char *path, CoordSys *root){
	init(path, root);
}

void CoordSys::init(const char *path, CoordSys *root){
	CoordSys *ret = this;
	const char *p, *name;
	if(path && (p = strrchr(path, '/'))){
		root = findcsppath(root, path, p);
		name = p + 1;
	}
	else
		name = path;
	VECNULL(ret->pos);
	VECNULL(ret->velo);
	QUATIDENTITY(ret->qrot);
/*	MAT4IDENTITY(ret->rot);
	MAT4IDENTITY(ret->irot);*/
	VECNULL(ret->omg);
	ret->rad = 1e2;
	ret->parent = root;
	if(!name)
		name = "?";
	char *newname = new char[strlen(name) + 1];
	strcpy(newname, name);
	ret->name = newname;
	ret->fullname = NULL;
	ret->flags = 0;
	ret->vwvalid = 0;
	ret->w = NULL;
	ret->next = NULL;
	ret->nchildren = 0;
	ret->children = NULL;
	ret->aorder = NULL;
	ret->naorder = 0;
	legitimize_child();
}

CoordSys::~CoordSys(){
	if(name)
		delete[] name;
}


void CoordSys::startdraw(){
	CoordSys *cs;
	vwvalid = 0;
	for(cs = children; cs; cs = cs->next)
		cs->startdraw();
}

bool CoordSys::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(!strcmp(s, "name")){
		if(s = strtok(ps, " \t\r\n")){
			char *name;
			if(this->name)
				delete[] this->name;
			name = new char[strlen(s) + 1];
			strcpy(name, s);
			this->name = name;
		}
		return true;
	}
	else if(!strcmp(s, "fullname")){
		if(s = ps){
			fullname = new char[strlen(s) + 1];
			strcpy(const_cast<char *>(fullname), s);
		}
		return true;
	}
	else if(!strcmp(s, "pos")){
		pos[0] = calc3(const_cast<const char **>(&argv[1]), sc.vl, NULL);
		if(2 < argc)
			pos[1] = calc3(const_cast<const char **>(&argv[2]), sc.vl, NULL);
		if(3 < argc)
			pos[2] = calc3(const_cast<const char **>(&argv[3]), sc.vl, NULL);
		return true;
	}
	else if(!strcmp(s, "radius")){
		if(ps)
			rad = atof(ps);
		return true;
	}
	else if(!strcmp(s, "diameter")){
		if(argv[1])
			rad = .5 * calc3(const_cast<const char **>(&argv[1]), sc.vl, NULL);
	}
	else if(!strcmp(s, "parent")){
		CoordSys *cs2, *csret;
		if(ps){
			if(*ps == '/')
				cs2 = sc.root, ps++;
			else
				cs2 = sc.root;
			if((csret = cs2->findcspath(ps)) || (csret = cs2->findcs(ps)))
				adopt_child(csret);
//			else
//				CmdPrintf("%s(%ld): Unknown CoordSys: %s", sc->fname, sc->line, ps);
		}
		return true;
	}
	else if(!strcmp(s, "solarsystem")){
		/* solar system is by default extent and isolated */
		if(1 < argc){
			if(!strcmp(argv[1], "false"))
				flags &= ~CS_SOLAR;
			else
				flags |= CS_SOLAR | CS_EXTENT | CS_ISOLATED;
		}
		else
			flags |= CS_SOLAR | CS_EXTENT | CS_ISOLATED;
		if(flags & CS_SOLAR){
			CoordSys *eis = findeisystem();
			if(eis){
				eis->aorder = (CoordSys**)realloc(eis->aorder, (eis->naorder + 1) * sizeof *eis->aorder);
				eis->aorder[eis->naorder++] = this;
			}
		}
		return true;
	}
	return false;
}

bool CoordSys::getpath(char *buf, size_t size)const{
#if 1
	return !!getpathint(buf, size);
#else
	if(!parent)
		return true; // root node need no names
	size_t len = strlen(name), buflen = strlen(buf);
	if(size <= len + buflen + 2) // terminating null + path delimiter
		return false;
	memmove(&buf[len+1], buf, buflen + 1);
	memcpy(buf, name, len);
	buf[len] = '/';
	parent->getpath(buf, size);
	return true;
#endif
}

int CoordSys::getpathint(char *buf, size_t size)const{
	int ret = 0;
	if(!parent)
		return 0;
	ret = parent->getpathint(buf, size);
	buf[ret] = '/';
	strcpy(&buf[ret+1], name);
	return strlen(name) + 1 + ret;
}

CoordSys *CoordSys::findeisystem(){
	CoordSys *ret;
	for(ret = parent; ret; ret = ret->parent) if((ret->flags & (CS_EXTENT | CS_ISOLATED)) == (CS_EXTENT | CS_ISOLATED)){
		return ret;
	}
	return NULL; /* the root system should be EI system */
}


void CoordSys::deleteAll(CoordSys **pp){
	CoordSys *cs2;
	if(!*pp)
		return;
	while((*pp)->children)
		deleteAll(&(*pp)->children);
	cs2 = *pp;
	*pp = cs2->next;
	if(cs2->w && (void*)cs2->w != (void*)&cs2[1])
		free(cs2->w);
	free(cs2);
}

Vec3d CoordSys::calcPos(const Viewer &vw){
	if(vwvalid & VW_POS)
		return vwpos;
	vwvalid |= VW_POS;
	return vwpos = vw.cs->tocs(pos, parent);
}

double CoordSys::calcScale(const Viewer &vw){
	if(vwvalid & VW_SCALE)
		return vwscale;
	vwvalid |= VW_SCALE;
	return vwscale = rad * vw.gc->scale(calcPos(vw));
}

