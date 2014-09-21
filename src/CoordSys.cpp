/** \file
 * \brief Implementation of CoordSys class.
 */
#define _CRT_SECURE_NO_WARNINGS

#include "Application.h"
#include "CoordSys.h"
#include "CoordSys-find.h"
#include "astro.h"
#include "Viewer.h"
#include "Player.h"
#include "stellar_file.h"
#include "war.h"
#include "Entity.h"
#include "cmd.h"
#include "serial_util.h"
#include "sqadapt.h"
#include "sqserial.h"
#include "Universe.h"
#ifndef DEDICATED
#include "draw/WarDraw.h"
#endif
#include "Game.h"
#include "CoordSys-property.h"
#include "CoordSys-sq.h"
extern "C"{
#include <clib/c.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/avec4.h>
#include <clib/timemeas.h>
#include <clib/mathdef.h>
}
#include <cpplib/gl/cullplus.h>
#include <stddef.h>
#include <assert.h>
#include <algorithm>
#include <string>
#include <iomanip>


long tocs_invokes = 0;
long tocs_parent_invokes = 0;
long tocs_children_invokes = 0;




void CoordSys::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << name;
	sc.o << fullname;
	sc.o << parent;
	sc.o << children;
	sc.o << w;
	sc.o << next;
	sc.o << pos << velo << rot;
	sc.o << omg;
	sc.o << csrad;
	sc.o << flags;
}

void CoordSys::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> name;
	sc.i >> fullname;
	sc.i >> parent;
	sc.i >> children;
	sc.i >> w;
	sc.i >> next;
	sc.i >> pos >> velo >> rot;
	sc.i >> omg;
	sc.i >> csrad;
	sc.i >> flags;

	CoordSys *eis = findeisystem();
	if(eis)
		eis->addToDrawList(this);

	// Clear the previous aorder list prior to unserialize children.
	// The dive() function must keep the children processed at last to make this method working.
	//
	// This is necessary when an element in the list can be transitted to or "grafted" to another node
	// in the process of simulation in the server.
	// For example, warp bubbles are the case because they can transit across EI-system and the client has
	// no way to know that happens (other than simulating the client copy of the universe, which can get
	// incomplete over time evolution).
	//
	// The obvious thing is that the client has responsibility to maintain the aorder list, because the
	// order depends on the observer's position. In other words, the client can do whatever it wants with
	// the list, regardless of the server's will.
	//
	// TODO: Clearing and reconstructing the list every frame is way too costly for such a rare event like
	// CoordSys transition. Probably some event handling mechanisms are preferred.
	if((flags & (CS_EXTENT | CS_ISOLATED)) == (CS_EXTENT | CS_ISOLATED))
		aorder.clear();
}

void CoordSys::dive(SerializeContext &sc, void (Serializable::*method)(SerializeContext &)){
	(this->*method)(sc);
	if(w)
		w->dive(sc, method);
	if(next)
		next->dive(sc, method);
	if(children)
		children->dive(sc, method);
}

//static ClassRegister<CoordSys> classRegister("CoordSys");
const CoordSys::Static CoordSys::classRegister("CoordSys", NULL, Conster<CoordSys>, st::Conster<CoordSys>, "CoordSys", sq_define);
//const CoordSys::Register<CoordSys> CoordSys::classRegister("CoordSys");

CoordSys **CoordSys::legitimize_child(){
	if(parent){
		CoordSys **ppcs;
		for(ppcs = &parent->children; *ppcs; ppcs = &(*ppcs)->next) if(*ppcs == this)
			return ppcs; /* i was already legitimized! */

		/* append the child into family list */
		next = parent->children;
		parent->children = this;
		onChangeParent(NULL);
		return &parent->children;
	}
	return NULL;
}

void CoordSys::adopt_child(CoordSys *newparent, bool retain){
	CoordSys *adoptee = this;
	CoordSys **ppcs2;
	Vec3d pos, velo;
	Quatd q;

	/* already adopted */
	if(adoptee->parent == newparent)
		return;

	// Obtain relative rotation matrix from old node to new node.
	if(retain){
		q = adoptee->tocsq(newparent); // This seems inverse to me, but it works.

	/* calculate relativity */
		pos = newparent->tocs(adoptee->pos, adoptee->parent);
		velo = newparent->tocsv(adoptee->velo, adoptee->pos, adoptee->parent);
	}

	/* remove from old family's list because one cannot be child of 2 or more
	  ancestory. this is one of the few points that is different from real
	  family tree; there's no marriage.
	   If the child is orphan, no settlement is needed. */
	if(adoptee->parent) for(ppcs2 = &adoptee->parent->children; *ppcs2; ppcs2 = &(*ppcs2)->next) if(*ppcs2 == adoptee){
		*ppcs2 = adoptee->next;
		break;
	}

	if(retain){
		adoptee->pos = pos;
		adoptee->velo = velo;
		adoptee->omg = q.trans(adoptee->omg);
		adoptee->rot = q;
	}

	/* recognize the new parent */
	CoordSys *oldParent = adoptee->parent;
	adoptee->parent = newparent;

	/* append the child into new family list */
	adoptee->next = newparent->children;
	newparent->children = adoptee;

	adoptee->onChangeParent(oldParent);
}

void CoordSys::onChangeParent(CoordSys *oldParent){}



static int findchild(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, const CoordSys *skipcs, bool delta){
	CoordSys *cs2;
	tocs_children_invokes++;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
		Vec3d v1, v;
		if(delta){
			v = cs2->rot.itrans(src);
		}
		else{
			v1 = src - cs2->pos;
			v = cs2->rot.itrans(v1);
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

static int findparent(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, bool delta){
	Vec3d v1, v;
	tocs_parent_invokes++;

	if(!cs->parent)
		return 0;

	if(delta){
		v = cs->rot.trans(src);
	}
	else{
		v1 = cs->rot.trans(src);
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

Vec3d CoordSys::tocs(const Vec3d &src, const CoordSys *cs, bool delta)const{
	Vec3d ret;
	tocs_invokes++;
	if(cs == this)
		ret = src;
	else if(findchild(ret, this, src, cs, NULL, delta))
		return ret;
	else if(!findparent(ret, this, src, cs, delta))
		ret = src;
	return ret;
}

static int findchildv(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const Vec3d &srcpos, const CoordSys *cs, const CoordSys *skipcs){
#if 1
	CoordSys *cs2;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
#else
	int i;
	for(i = 0; i < cs->nchildren; i++) if(cs->children[i] && cs->children[i] != skipcs){
		const coordsys *cs2 = cs->children[i];
#endif
		/* position */
		Vec3d pos = cs2->rot.itrans(srcpos - cs2->pos);

		/* velocity */
		Vec3d v1 = src - cs2->velo;
		Vec3d vrot = cs2->rot.itrans(cs2->omg).vp(pos); // Centrifugal force
		v1 -= vrot;
		Vec3d v = cs2->rot.itrans(v1);

		if(retcs == cs2){
			ret = v;
			return 1;
		}
		if(findchildv(ret, retcs, v, pos, cs2, NULL))
			return 1;
	}
	return 0;
}

static int findparentv(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const Vec3d &srcpos, const CoordSys *cs){
	if(!cs->parent)
		return 0;

	/* velocity */
	Vec3d v = cs->rot.trans(src);
	v += cs->velo;
	Vec3d vrot = /*cs->rot.trans*/(cs->omg).vp(cs->rot.trans(srcpos));
	Vec3d v1 = /*cs->rot.trans*/(vrot);
	v += v1;

	if(cs->parent == retcs){
		ret = v;
		return 1;
	}

	/* position */
	Vec3d pos = cs->rot.trans(srcpos) + cs->pos;

	/* do not scan subtrees already checked! */
	if(findchildv(ret, retcs, v, pos, cs->parent, cs))
		return 1;
	return findparentv(ret, retcs, v, pos, cs->parent);
}

Vec3d CoordSys::tocsv(const Vec3d &src, const Vec3d &srcpos, const CoordSys *cs)const{
	Vec3d ret;
	if(cs == this)
		return src;
	else if(findchildv(ret, this, src, srcpos, cs, NULL))
		return ret;
	else if(findparentv(ret, this, src, srcpos, cs))
		return ret;
	return src;
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
		q = src * cs2->rot;
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

	q = src.imul(cs->rot);

	if(cs->parent == retcs){
		ret = q;
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




bool CoordSys::belongs(const Vec3d &pos)const{
	if(!parent)
		return true;
	return pos.slen() < csrad * csrad;
}

CoordSys *CoordSys::findchildb(Vec3d &ret, const Vec3d &src, const CoordSys *skipcs)const{
	CoordSys *cs2;
	for(cs2 = children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
		Vec3d v1, v;
		v1 = src - cs2->pos;
		v = cs2->rot.itrans(v1);
/*		mat4vp3(v, cs2->irot, v1);*/

		/* radius is measured in parent coordinate system and if the position is not within it,
		  we do not belong to this one. */
		if(cs2->belongs(v)){
			CoordSys *csret;
			if(csret = cs2->findchildb(ret, v, NULL))
				return csret;
			ret = v;
			return cs2;
		}
	}
	return NULL;
}

const CoordSys *CoordSys::findparentb(Vec3d &ret, const Vec3d &src)const{
	const CoordSys *cs2 = this->parent;

	/* if it is root node, we belong to it no matter how its radius is */
	if(!this->parent || belongs(src)){
		ret = src;
		return this;
	}

	Vec3d v1 = this->rot.trans(src);
	Vec3d v = v1 + this->pos;

	/* if the position vector exceeds sphere's radius, check for parents.
	  otherwise, check for all the children. */
	if(this->parent->belongs(src)){
		CoordSys *csret = parent->findchildb(ret, v, this);
		/* do not scan subtrees already checked! */
		if(csret)
			return csret;

		/* if no children has the vector in its volume, it's for parent. */
		ret = v;
		return cs2;
	}
	return parent->findparentb(ret, v);
}

const CoordSys *CoordSys::belongcs(Vec3d &ret, const Vec3d &src)const{
	CoordSys *csret;
	if(csret = findchildb(ret, src, NULL))
		return csret;
	else if(belongs(src))
		return this;
	else
		return findparentb(ret, src);
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
	if(this->fullname && !strcmp(this->fullname, name))
		return this;
	for(int i = 0; i < extranames.size(); i++)
		if(extranames[i] == name)
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
	if(path == p){
		CoordSys *root;
		for(root = this; root->parent; root = root->parent);
		return root->findcspath(path+1);
	}
	if(!p)
		p = &path[strlen(path)];
	if(!strncmp(".", path, p - path)){
		if(!*p)
			return this;
		else
			return findcspath(p+1);
	}
	else if(!strncmp("..", path, p - path)){
		if(!*p)
			return this->parent;
		else
			return parent->findcspath(p+1);
	}
	if(p) for(cs = this->children; cs; cs = cs->next) if(cs->name.len() == p - path && !strncmp(cs->name, path, p - path)){
		if(!*p)
			return cs;
		else
			return cs->findcspath(p+1);
	}
	return NULL;
}

/* partial path */
CoordSys *CoordSys::findcsppath(const char *path, const char *pathend){
	CoordSys *root = this;
	CoordSys *cs;
	const char *p;
	if(!*path || 0 <= path - pathend)
		return root;
	p = strchr(path, '/');
	if(!p || pathend - p < 0)
		p = pathend;
	if(p) for(cs = root->children; cs; cs = cs->next) if(!strncmp(cs->name, path, p - path))
		return cs->findcsppath(p+1, pathend);
	return NULL;
}

/// This template can have either one of the argument pairs:
/// <CoordSys, FindCallback> or <const CoordSys, FindCallbackConst>.
template<typename T, typename Callback>
bool CoordSys::tempFindChild(T *cs, Callback &fc, T *skipcs){
	for(T *cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
		if(!fc.invoke(cs2))
			return false;
		tempFindChild<T, Callback>(cs2, fc, NULL);
	}
	return true;
}

/// This template can have either one of the argument pairs:
/// <CoordSys, FindCallback> or <const CoordSys, FindCallbackConst>.
template<typename T, typename Callback>
bool CoordSys::tempFindParent(T *cs, Callback &fc){
	if(!cs)
		return false;
	if(!tempFindChild(cs, fc, cs))
		return false;
	// This invocation could be before findchild().
	fc.invoke(cs);
	return tempFindParent(cs->parent, fc);
}

bool CoordSys::find(FindCallback &fc){
	return tempFindParent(this, fc);
}

bool CoordSys::find(FindCallbackConst &fc)const{
	return tempFindParent(this, fc);
}

CoordSys::FindCallbackConst::~FindCallbackConst(){}

static std::map<double, CoordSys*> drawnlist;

void CoordSys::predraw(const Viewer *vw){
	drawnlist.clear();
	for(CoordSys *cs = children; cs; cs = cs->next)
		cs->predraw(vw);
}

// In the hope std::sort template function optimizes the comparator function,
// which is not expected in the case of qsort().
class AstroCmp{
	const Viewer &vw;
public:
	static int invokes;
	AstroCmp(const Viewer &avw) : vw(avw){}
	bool operator()(CoordSys *const &a, CoordSys *const &b){
		double ad, bd;
		Astrobj *aa = a->toAstrobj(), *ab = b->toAstrobj();
		invokes++;
		if(!a) return true;
		else if(!b) return false;
		ad = a->calcSDist(vw) - (aa ? aa->rad * aa->rad : a->csrad * a->csrad);
		bd = b->calcSDist(vw) - (ab ? ab->rad * ab->rad : b->csrad * b->csrad);
		return ad < bd ? true : bd < ad ? false : false;
	}
};
int AstroCmp::invokes = 0;

void CoordSys::draw(const Viewer *vw){
}
void CoordSys::drawtra(const Viewer *vw){}

void CoordSys::drawcs(const Viewer *vw){
			timemeas_t tm;
			TimeMeasStart(&tm);
	draw(vw);
	drawnlist[TimeMeasLap(&tm)] = this;
//		printf("drawcs1 %d %lg %s\n", AstroCmp::invokes, TimeMeasLap(&tm), (const char*)getpath());
	Vec3d cspos;
	cspos = vw->cs->tocs(avec3_000, this);
/*	if(cs->flags & CS_EXTENT && vw->gc && glcullFrustum(cspos, cs->rad, vw->gc))
		return;*/
	if((CS_EXTENT | CS_ISOLATED) == (flags & (CS_EXTENT | CS_ISOLATED))){
		{
//			timemeas_t tm;
//			TimeMeasStart(&tm);
		std::sort(aorder.begin(), aorder.end(), AstroCmp(*vw));
//		printf("sortcs %d %d %lg\n", AstroCmp::invokes, aorder.size(), TimeMeasLap(&tm));
		}
		AOList::reverse_iterator i = aorder.rbegin();
//			timemeas_t tm;
//			TimeMeasStart(&tm);
		for(; i != aorder.rend();i++) if(*i){
			if(*i == this){
				(*i)->draw(vw);
			}
			else
				(*i)->drawcs(vw);
		}
//		printf("drawcs %d %lg %s\n", AstroCmp::invokes, TimeMeasLap(&tm), (const char*)getpath());
	}
}

void CoordSys::drawWar(WarDraw *wd){
	;
}

extern double get_timescale();
void CoordSys::anim(double dt){
	Vec3d omgdt;
/*	extern double get_timescale();
	dt *= get_timescale();*/
	if(0 < this->omg.slen()){
		double scale = game->universe->astro_timescale;
		double len = this->omg.len();
		this->rot = Quatd::rotation(len * dt * scale, this->omg / len) * this->rot;
	}
//	omgdt = omg * dt;
//	rot = rot.quatrotquat(omgdt);
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next)
		cs->anim(dt);
	if(w)
		w->anim(dt);
}

void CoordSys::clientUpdate(double dt){
	if(0 < this->omg.slen()){
		double scale = game->universe->astro_timescale;
		double len = this->omg.len();
		this->rot = Quatd::rotation(len * dt * scale, this->omg / len) * this->rot;
	}
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next)
		cs->clientUpdate(dt);
	if(w)
		w->clientUpdate(dt);
}

void CoordSys::postframe(){
	if(w)
		w->postframe();
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next)
		cs->postframe();
}
void CoordSys::endframe(){
	vwvalid = 0;
	CoordSys *cs;
	for(cs = children; cs;){
		CoordSys *csnext = cs->next; // cs can unlink itself from the children list
		cs->endframe();
		cs = csnext;
	}

	// delete offsprings first to avoid adoption of children as possible.
	// Actually, only the server can delete a CoordSys.
	if(game->isServer() && flags & CS_DELETE){
		// unlink from parent's children list
		if(parent){
			bool ok = false;
			for(CoordSys **ppcs = &parent->children; *ppcs; ppcs = &(*ppcs)->next) if(*ppcs == this){
				*ppcs = next;
				ok = true;
				break;
			}
			assert(ok);
		}

		// Remove myself from astrobj sorted list.
		if(CoordSys *eis = findeisystem()){
			for(AOList::iterator it = eis->aorder.begin(); it != eis->aorder.end(); it++) if(*it == this){
				eis->aorder.erase(it);
				break;
			}
		}

		// give children to parent.
		for(cs = children; cs;){
			CoordSys *csnext = cs->next;
			cs->velo = parent->tocsv(cs->velo, cs->pos, this);
			cs->pos = parent->tocs(cs->pos, this);
			cs->omg = parent->tocs(cs->omg, this, true);
			cs->rot = parent->tocsq(this);
			cs->parent = parent;
			cs->next = parent->children;
			parent->children = cs;
			cs = csnext;
		}

		// rest in peace..
		delete this;
	}
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


/// \brief The constructor for reserving space for unserialization.
CoordSys::CoordSys(){
	init(NULL, NULL);
}

/// \brief The constructor for the real object.
CoordSys::CoordSys(const char *path, CoordSys *root) : st(root ? root->getGame() : NULL){
	init(path, root);
}

/// \brief The constructor that only Universe uses (apparently).
CoordSys::CoordSys(Game *game) : st(game){
	init(NULL, NULL);
}

void CoordSys::init(const char *path, CoordSys *root){
	CoordSys *ret = this;
	const char *p, *name;
	if(path && (p = strrchr(path, '/'))){
		root = root->findcsppath(path, p);
		name = p + 1;
	}
	else
		name = path;
	ret->pos.clear();
	ret->velo.clear();
	ret->rot = quat_u;
	ret->omg.clear();
	ret->csrad = 1e2;
	ret->parent = root;
	if(!name)
		name = "?";
	ret->name = name;
	ret->fullname = "";
	ret->flags = 0;
	ret->vwvalid = 0;
	ret->w = NULL;
	ret->next = NULL;
	ret->children = NULL;
	ret->naorder = 0;
	legitimize_child();
}

extern int 	cs_destructs;
int cs_destructs = 0;

CoordSys::~CoordSys(){
//	delete children;
	// Do not delete siblings, they may be alive after this death.
//	delete next;
	cs_destructs++;
	if(game->isServer()){
		for(CoordSys *cs = children; cs;){
			CoordSys *csnext = cs->next;
			delete cs;
			cs = csnext;
		}
		delete w;
	}

	// Unregister itself from the nearest enclosing extent-isolated system.
	// The containing system has a strong reference to this object, so just destroying the object
	// does not automatically clears it, thus we must manually erase it.
	// This happens no matter server or client.
	CoordSys *eis = findeisystem();
	if(eis){
		for(AOList::iterator it = eis->aorder.begin(); it != eis->aorder.end(); ++it){
			if(*it == this){
				eis->aorder.erase(it);
				break;
			}
		}
	}
}


void CoordSys::startdraw(){
	CoordSys *cs;
	vwvalid = 0;
	for(cs = children; cs; cs = cs->next)
		cs->startdraw();
}

bool CoordSys::readFileStart(StellarContext &){
	return true;
}

std::map<const CoordSys *, std::vector<dstring> > linemap;

bool CoordSys::readFile(StellarContext &sc, int argc, const char *argv[]){
	const char *s = argv[0], *ps = argv[1];
	if(!strcmp(s, "name")){
		if(ps)
			name = ps;
		return true;
	}
	else if(!strcmp(s, "fullname")){
		if(ps)
			fullname = ps;
		return true;
	}
	else if(!strcmp(s, "extraname")){
		if(ps)
			extranames.push_back(ps);
		return true;
	}
	else if(!strcmp(s, "pos")){
		pos[0] = sqcalc(sc, argv[1], s);
		if(2 < argc)
			pos[1] = sqcalc(sc, argv[2], s);
		if(3 < argc)
			pos[2] = sqcalc(sc, argv[3], s);
		return true;
	}
	else if(!strcmp(s, "galactic_coord")){
		double lon = sqcalc(sc, argv[1], s) / deg_per_rad;
		double lat = 2 < argc ? sqcalc(sc, argv[2], s) / deg_per_rad : 0.;
		double dist = 3 < argc ? sqcalc(sc, argv[3], s) : 1e10;
		pos = dist * Vec3d(sin(lon) * cos(lat), cos(lon) * cos(lat), sin(lat));
		return true;
	}
	else if(!strcmp(s, "cs_radius")){
		if(ps)
			csrad = sqcalc(sc, ps, s);
		return true;
	}
	else if(!strcmp(s, "cs_diameter")){
		if(argv[1])
			csrad = .5 * sqcalc(sc, argv[1], s);
		return true;
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
	else if(!strcmp(s, "addent")){
		WarField *w;
		Entity *pt;
		if(this->w)
			w = this->w;
		else
			w = this->w = new WarSpace(this)/*spacewar_create(cs, ppl)*/;
		pt = Entity::create(argv[1], w);
		if(pt){
			pt->pos[0] = 2 < argc ? sqcalc(sc, argv[2], s) : 0.;
			pt->pos[1] = 3 < argc ? sqcalc(sc, argv[3], s) : 0.;
			pt->pos[2] = 4 < argc ? sqcalc(sc, argv[4], s) : 0.;
			pt->race = 5 < argc ? atoi(argv[5]) : 0;
		}
		else
			printf("%s(%ld): addent: Unknown entity class name: %s\n", sc.fname, sc.line, argv[1]);
		return true;
	}
	// Specifying an Entity type directly is obsolete. You can use Squirrel codes to extend
	// the specification of SSD files. Respawn points are not actively used anyway.
#if 0
	else if(!strcmp(s, "respawn")){
		if(argc < 2)
			return false;
		extern Player *ppl;
		WarField *w;
		Entity *pt;
		if(this->w)
			w = this->w;
		else
			w = this->w = new WarSpace(this)/*spacewar_create(cs, ppl)*/;
		const char *classname = 1 < argc ? argv[1] : NULL;
		Vec3d pos(2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0.,
			3 < argc ? calc3(&argv[3], sc.vl, NULL) : 0.,
			4 < argc ? calc3(&argv[4], sc.vl, NULL) : 0.);
		int ind = 4;
		int race = ++ind < argc ? int(calc3(&argv[ind], sc.vl, NULL)) : 0;
		double interval = ++ind < argc ? calc3(&argv[ind], sc.vl, NULL) : 1.;
		int max_count = ++ind < argc ? int(calc3(&argv[ind], sc.vl, NULL)) : 1;
		double initial_phase = ++ind < argc ? calc3(&argv[ind], sc.vl, NULL) : 1.;
		pt = new Respawn(w, interval, initial_phase, max_count, classname);
		if(pt){
			pt->pos = pos;
			pt->race = race;
			w->addent(pt);
		}
		return true;
	}
#endif
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
				eis->addToDrawList(this);
			}
		}
		return true;
	}
	else if(!strcmp(s, "eisystem")){
		/* solar system is by default extent and isolated */
		if(1 < argc){
			if(!strcmp(argv[1], "false"))
				flags &= ~CS_SOLAR;
			else
				flags |= CS_EXTENT | CS_ISOLATED;
		}
		else
			flags |= CS_EXTENT | CS_ISOLATED;
		if(flags & CS_EXTENT){
			CoordSys *eis = findeisystem();
			if(eis)
				eis->addToDrawList(this);
		}
		return true;
	}
	else if(!strcmp(s, "rotation")){
		if(argc == 2){
			StackReserver st2(sc.v);
			cpplib::dstring dst = cpplib::dstring("return(") << argv[1] << ")";
			if(SQ_SUCCEEDED(sq_compilebuffer(sc.v, dst, dst.len(), _SC("rotation"), SQTrue))){
				sq_push(sc.v, -2);
				if(SQ_SUCCEEDED(sq_call(sc.v, 1, SQTrue, SQTrue))){
					sqa::SQQuatd q;
					q.getValue(sc.v, -1);
					rot = q.value;
				}
			}
			return true;
		}
		if(1 < argc)
			rot[0] = sqcalc(sc, argv[1], s);
		if(2 < argc)
			rot[1] = sqcalc(sc, argv[2], s);
		if(3 < argc){
			rot[2] = sqcalc(sc, argv[3], s);
			rot[3] = sqrt(1. - VECSLEN(rot));
		}
		return true;
	}
	else if(!strcmp(s, "omega")){
		if(argc == 2){
			StackReserver st2(sc.v);
			cpplib::dstring dst = cpplib::dstring("return(") << argv[1] << ")";
			if(SQ_SUCCEEDED(sq_compilebuffer(sc.v, dst, dst.len(), _SC("omega"), SQTrue))){
				sq_push(sc.v, -2);
				if(SQ_SUCCEEDED(sq_call(sc.v, 1, SQTrue, SQTrue))){
					sqa::SQVec3d q;
					q.getValue(sc.v, -1);
					omg = q.value;
				}
			}
			return true;
		}
		if(1 < argc)
			omg[0] = sqcalc(sc, argv[1], s);
		if(2 < argc)
			omg[1] = sqcalc(sc, argv[2], s);
		if(3 < argc)
			omg[2] = sqcalc(sc, argv[3], s);
		if(4 < argc){
			double d;
			omg.normin();
			d = sqcalc(sc, argv[4], s);
			omg.scalein(d);
		}
		return true;
	}
	else if(!strcmp(s, "updirection")){
		Vec3d v = vec3_000;
		if(1 < argc)
			v[0] = sqcalc(sc, argv[1], s);
		if(2 < argc)
			v[1] = sqcalc(sc, argv[2], s);
		if(3 < argc)
			v[2] = sqcalc(sc, argv[3], s);
		rot = Quatd::direction(v);
		if(rot.slen() == 0.){
			CmdPrint("Quaternion zero!");
			rot = Quatd(0,0,0,1);
		}
		return true;
	}
	else if(!strcmp(s, "binds")){
//		CoordSys *cs = this->findcspath(argv[1]);
//		if(cs)
//			linemap[this].push_back(cs);
		linemap[this].push_back(argv[1]);
		return true;
	}
	else{ // An undefined parameter name is passed to Squirrel extension code.
		HSQUIRRELVM v = sc.v;
		StackReserver sr(v);

		// This code is somewhat similar to sqa_console_command.
		try{
			// Try to delegate the directive to a Squirrel function.
			sq_pushroottable(v);
			sq_pushstring(v, _SC("stellarReadFile"), -1);
			if(SQ_FAILED(sq_get(v, -2))) // stellarReadFile
				throw sqa::SQFError(_SC("readFile key is not defined in CoordSys"));
			sq_pushroottable(v); // stellarReadFile root
			CoordSys::sq_pushobj(v, this); // stellarReadFile root cs

			// Pass current variables table
			sq_pushobject(v, sc.vars); // stellarReadFile root cs vars

			// Pass all arguments as strings (no conversion is done beforehand).
			for(int i = 0; i < argc; i++)
				sq_pushstring(v, argv[i], -1);

			// Call the function readFile with arguments (root, cs, defineTable, ...)
			// It's no use examining returned value in that case calling the function iteslf fails.
			if(SQ_FAILED(sq_call(v, argc+3, SQTrue, SQTrue)))
				throw sqa::SQFError(_SC("readFile function could not be called"));

			// Assume returned value integer.
			SQInteger retint;
			if(SQ_SUCCEEDED(sq_getinteger(v, -1, &retint)) && retint != 0)
				return true;
		}
		catch(sqa::SQFError &e){
			CmdPrint(cpplib::dstring() << "CoordSys::readFile: " << e.description);
		}
	}
	return false;
}

bool CoordSys::readFileEnd(StellarContext &){
	return true;
}

Astrobj *CoordSys::toAstrobj(){
	return NULL;
}

OrbitCS *CoordSys::toOrbitCS(){
	return NULL;
}

Universe *CoordSys::toUniverse(){
	return NULL;
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
	return name.len() + 1 + ret;
}

cpplib::dstring CoordSys::getpath()const{
	cpplib::dstring ret;
	getpathint(ret);
	return ret;
}

int CoordSys::getpathint(cpplib::dstring &ret)const{
	if(!this){
		ret = "(null)";
		return 0;
	}
	if(!parent){
		ret = '/';
		return 0;
	}
	int depth = parent->getpathint(ret);

	// Merge path delimiters
	if(ret[ret.len()-1] != '/')
		ret << '/';

	ret << name;
	return depth+1;
}



struct StackTrace{
	StackTrace *prev;
	const CoordSys *base;
};

static bool findcsrpathchildren(const CoordSys *subject, StackTrace *pst, const CoordSys *ignore, cpplib::dstring &ret){
	const CoordSys *cs;
	if(subject == pst->base){
		for(StackTrace *ist = pst; pst; pst = pst->prev){
			if(ist != pst)
				ret.strcat("/");
			ret.strcat(pst->base->name);
		}
		return true;
	}
	for(cs = pst->base->children; cs; cs = cs->next){
		StackTrace st;
		st.prev = pst;
		st.base = cs;
		if(findcsrpathchildren(subject, &st, NULL, ret))
			return true;
	}
/*		if(subject = cs){
		ret.strcat("/");
		ret.strcat(cs->name);
		return true;
	}*/
	return false;
}

static bool findcsrpath(const CoordSys *subject, const CoordSys *base, cpplib::dstring &ret){
//	CoordSys *cs;
//	const char *p;
	if(!base)
		return true;
	if(subject == base){
		ret.strcat("/");
		ret.strcat(base->name);
		return true;
	}
/*	bool cr = findcsrpathchildren(subject, base, ret);
	if(cr)
		return true;
	if(p) for(cs = this->children; cs; cs = cs->next) if(strlen(cs->name) == p - path && !strncmp(cs->name, path, p - path)){
		if(!*p)
			return cs;
		else
			return cs->findcspath(p+1);
	}*/
	return NULL;
}


cpplib::dstring CoordSys::getrpath(const CoordSys *base)const{
	cpplib::dstring ret;
	StackTrace st;
	st.prev = NULL;
	st.base = base;
	findcsrpathchildren(this, &st, NULL, ret);
	return ret;
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

static Player *obtainPlayer(void *pvapp){
	Application *app = (Application*)pvapp;
	if(!app){
		CmdPrint("ERROR: Application object could not be obtained.");
		return NULL;
	}
	Game *game = app->clientGame ? app->clientGame : app->serverGame;
	if(!game){
		CmdPrint("ERROR: Game object could not be obtained.");
		return NULL;
	}
	Player *player = game->player;
	if(!player)
		CmdPrint("ERROR: Player object could not be obtained.");
	return player;
}

static int cmd_ls(int argc, char *argv[], void *pv){
	Player *ppl = obtainPlayer(pv);
	if(!ppl){
		return 1;
	}
	if(argc <= 1){
		for(CoordSys *cs = ppl->cs->children; cs; cs = cs->next){
			cpplib::dstring ds = cs->getrpath(cs);
			CmdPrint(ds);
		}
		return 0;
	}
	const CoordSys *parent;
	if(parent = ppl->cs->findcspath(argv[1])){
		for(const CoordSys *cs = parent->children; cs; cs = cs->next){
			cpplib::dstring ds = cs->getrpath(cs);
			CmdPrint(ds);
		}
	}
	else
		CmdPrint(cpplib::dstring() << "Could not find path " << argv[1]);
	return 0;
}

static int cmd_ll(int argc, char *argv[], void *pv){
	Player *ppl = obtainPlayer(pv);
	if(!ppl){
		return 1;
	}
	if(argc <= 1){
		for(CoordSys *cs = ppl->cs->children; cs; cs = cs->next){
			cpplib::dstring ds = cs->getrpath(cs) << ": " << cs->classname();
			CmdPrint(ds);
		}
		return 0;
	}
	const CoordSys *parent;
	if(parent = ppl->cs->findcspath(argv[1])){
		for(const CoordSys *cs = parent->children; cs; cs = cs->next){
			cpplib::dstring ds = cs->getrpath(cs) << ": " << cs->classname();
			CmdPrint(ds);
		}
	}
	else
		CmdPrint(cpplib::dstring() << "Could not find path " << argv[1]);
	return 0;
}

static int cmd_pwd(int argc, char *argv[], void *pv){
	Player *ppl = obtainPlayer(pv);
	if(!ppl){
		return 1;
	}
	CmdPrint(ppl->cs->getpath());
	return 0;
}

CoordSys::CtorMap &ctormap();

static int cmd_cslist(int argc, char *argv[], void *pv){
	CoordSys::CtorMap &cm = ctormap();
	for(CoordSys::CtorMap::iterator it = cm.begin(); it != cm.end(); it++){
		CmdPrint(it->first);
	}
	CmdPrint(cpplib::dstring() << cm.size() << " CoordSys-derived classes are listed");
	return 0;
}

bool CoordSys::registerCommands(Application *app){
	CmdAddParam("ls", cmd_ls, app);
	CmdAddParam("ll", cmd_ll, app);
	CmdAddParam("pwd", cmd_pwd, app);
	CmdAddParam("cslist", cmd_cslist, app);
	return true;
}


CoordSys::CtorMap &ctormap(){
	static CoordSys::CtorMap map;
	return map;
}

const CoordSys::CtorMap &CoordSys::ctormap(){
	return ::ctormap();
}

unsigned CoordSys::registerClass(Static &s){
	st::registerClass(s.id, s.stconstruct);
	CtorMap &cm = ::ctormap();
	if(cm.find(s.id) != cm.end())
		CmdPrint(cpplib::dstring("WARNING: Duplicate class name: ") << s.id);
	cm[s.id] = &s;

	// If an extension module is loaded and a CoordSys derived class is newly defined through this function,
	// the Squirrel VM is already initialized to load definition of classes.
	// Therefore, extension module classes must define themselves in Squirrel code as they register.
	if(sqa::g_sqvm){
		sq_pushroottable(g_sqvm);
		s.sq_define(sqa::g_sqvm);
		sq_poptop(g_sqvm);
	}

	return cm.size();
}

void CoordSys::unregisterClass(ClassId id){
	st::unregisterClass(id);
	::ctormap().erase(id);
}

const SQChar *objectsTableName = _SC("objects"); // This name could be controversial

const SQInteger CoordSys::sq_udsize = sizeof(SqSerialPtr<CoordSys>);

/// \brief The release hook of Entity that clears the weak pointer.
///
/// \param size is always 0?
static SQInteger sqh_release(SQUserPointer p, SQInteger size){
	((SqSerialPtr<CoordSys>*)p)->~SqSerialPtr<CoordSys>();
	return 1;
}

// TODO: deleted object's entry should be removed
void sqserial_findobj(HSQUIRRELVM v, Serializable *s, void create(HSQUIRRELVM v, Serializable *cs)){
	if(!s){
		sq_pushnull(v);
		return;
	}
	sq_pushroottable(v); // root
	sq_pushstring(v, objectsTableName, -1); // root str
	if(SQ_FAILED(sq_get(v, -2))){ // root table
		sq_newtable(v); // root {}
		sq_pushroottable(v); // root {} root
		sq_pushstring(v, objectsTableName, -1); // root {} root str
		sq_push(v, -3); // root {} root str {}
		sq_newslot(v, -3, SQFalse); // root {} root
		sq_poptop(v); // root {}
	}
	sq_pushinteger(v, s->getid()); // root {} id
	if(SQ_FAILED(sq_get(v, -2))){ // root {} obj
		create(v, s);

		sq_pushinteger(v, s->getid()); // root {} obj id
		sq_push(v, -2); // root {} obj id obj
		sq_newslot(v, -4, SQFalse); // root {id=obj} obj
	}
	sq_remove(v, -2); // Remove coordSysObjects table
	sq_remove(v, -2); // Remove root table
}

/// \brief Callback function that actually pushes the CoordSys
static void pushCoordSys(HSQUIRRELVM v, Serializable *s){
	CoordSys *cs = static_cast<CoordSys*>(s);
	sq_pushroottable(v);
	CoordSys::CtorMap::const_iterator it = CoordSys::ctormap().find(cs->classname());
	if(it == CoordSys::ctormap().end())
		throw SQFError(_SC("Error wrong classname"));
	sq_pushstring(v, it->second->s_sqclassname, -1);
	if(SQ_FAILED(sq_get(v, -2)))
		throw SQFError("Something's wrong with CoordSys class definition.");
	if(SQ_FAILED(sq_createinstance(v, -1)))
		throw SQFError(gltestp::dstring() << "Couldn't create class " << it->second->s_sqclassname);
	SQUserPointer p;
	if(SQ_FAILED(sq_getinstanceup(v, -1, &p, NULL)))
		throw SQFError("Something's wrong with Squirrel Class Instace of CoordSys.");
	new(p) SqSerialPtr<CoordSys>(v, cs);
	sq_setreleasehook(v, -1, sqh_release);
	sq_remove(v, -2); // Remove Class
	sq_remove(v, -2); // Remove root table
}

void CoordSys::sq_pushobj(HSQUIRRELVM v, CoordSys *cs){
	if(!cs)
		sq_pushnull(v);
	else
		sqserial_findobj(v, cs, pushCoordSys);
}

CoordSys *CoordSys::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, idx, &up, NULL)) || !up)
		return NULL;
	// Do not throw error here because it's not an error if a CoordSys is destroyed.
//		throw SQFError("Something's wrong with Squirrel Class Instace of CoordSys.");
	return *(SqSerialPtr<CoordSys>*)up;
}


/// It's shame
static SQInteger sqf_getclass(HSQUIRRELVM v){
	if(SQ_SUCCEEDED(sq_getclass(v,1)))
		return 1;
	return SQ_ERROR;
}

static SQInteger sqf_addent(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	CoordSys *p = CoordSys::sq_refobj(v);
	
	SQVec3d qv;
	qv.getValue(v, -1);
	Vec3d &pos = qv.value;

	const SQChar *arg;
	if(SQ_FAILED(sq_getstring(v, 2, &arg)))
		return SQ_ERROR;

	WarField *&w = p->w;
	if(!w)
		w = new WarSpace(p);
	Entity *pt = Entity::create(arg, w);
	if(pt){
		// Pass zero defaults to avoid accidental uninitialized values
		const Quatd rot = quat_u;
		const Vec3d velo(0,0,0);
		const Vec3d omg(0,0,0);
		pt->setPosition(&pos, &rot, &velo, &omg);
/*		pt->pos = pos;
		if(pt->bbody){
			btTransform trans;
			trans.setIdentity();
			trans.setOrigin(btvc(pos));
			pt->bbody->setCenterOfMassTransform(trans);
		}*/
/*		pt->race = 5 < argc ? atoi(argv[5]) : 0;*/
	}
	else
		return sq_throwerror(v, cpplib::dstring("addent: Unknown entity class name: ") << arg);

	Entity::sq_pushobj(v, pt);
	return 1;
}

static SQInteger sqf_transPosition(HSQUIRRELVM v){
	try{
		CoordSys *p = CoordSys::sq_refobj(v);
		CoordSys *p2 = CoordSys::sq_refobj(v, 3);
		SQBool delta;
		if(SQ_FAILED(sq_getbool(v, 4, &delta)))
			delta = SQFalse;
		SQVec3d qv;
		qv.getValue(v, 2);
		qv.value = p->tocs(qv.value, p2, !!delta);
		qv.push(v);
		return 1;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

static SQInteger sqf_transRotation(HSQUIRRELVM v){
	try{
		CoordSys *p = CoordSys::sq_refobj(v);
		CoordSys *p2 = CoordSys::sq_refobj(v, 2);
		Quatd q = p->tocsq(p2);
		SQQuatd qv(q);
		qv.push(v);
		return 1;
	}
	catch(SQFError &e){
		return sq_throwerror(v, e.what());
	}
}

static SQInteger sqf_getpath(HSQUIRRELVM v){
	CoordSys *p = CoordSys::sq_refobj(v);
	sq_pushstring(v, p->getpath(), -1);
	return 1;
}

static SQInteger sqf_findcs(HSQUIRRELVM v){
	CoordSys *p = CoordSys::sq_refobj(v);
	const SQChar *s;
	sq_getstring(v, -1, &s);
	CoordSys *cs = p->findcs(s);
	if(cs){
		CoordSys::sq_pushobj(v, cs);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

static SQInteger sqf_findcspath(HSQUIRRELVM v){
	CoordSys *p = CoordSys::sq_refobj(v);
	const SQChar *s;
	sq_getstring(v, -1, &s);
	CoordSys *cs = p->findcspath(s);
	if(cs){
		CoordSys::sq_pushobj(v, cs);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

template<int MASK>
static SQInteger sqFlagGetter(HSQUIRRELVM v, const CoordSys *cs){
	sq_pushbool(v, cs->flags & MASK);
	return 1;
}

template<int MASK>
static SQInteger sqFlagSetter(HSQUIRRELVM v, CoordSys *cs){
	SQBool b;
	if(SQ_FAILED(sq_getbool(v, 3, &b)))
		return sq_throwerror(v, _SC("Could not convert to bool"));
	if(b)
		cs->flags |= MASK;
	else
		cs->flags &= ~MASK;
	return 0;
}

const CoordSys::PropertyMap &CoordSys::propertyMap()const{
	static PropertyMap pmap;
	static bool propInit = false;
	if(!propInit){
		propInit = true;
		pmap[_SC("id")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){sq_pushinteger(v, cs->getid()); return SQInteger(1);}, NULL);
		pmap[_SC("name")] = PropertyEntry([](HSQUIRRELVM v, const CoordSys *cs){sq_pushstring(v, cs->name, -1); return SQInteger(1);}, NULL);
		pmap[_SC("pos")] = PropertyEntry(tgetter<Vec3d, &CoordSys::pos>, tsetter<Vec3d, &CoordSys::pos>);
		pmap[_SC("velo")] = PropertyEntry(tgetter<Vec3d, &CoordSys::velo>, tsetter<Vec3d, &CoordSys::velo>);
		pmap[_SC("rot")] = PropertyEntry(tgetter<Quatd, &CoordSys::rot>, tsetter<Quatd, &CoordSys::rot>);
		pmap[_SC("omg")] = PropertyEntry(tgetter<Vec3d, &CoordSys::omg>, tsetter<Vec3d, &CoordSys::omg>);
		pmap[_SC("child")] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				sq_pushobj(v, cs->children);
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				// TODO: Scripts can screw up CoordSys tree structure by assigning invalid value to child.
				// Probably we should return a list to Squirrel VM.
				cs->children = sq_refobj(v, 3);
				return SQInteger(0);
			}
		);
		pmap[_SC("next")] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				sq_pushobj(v, cs->next);
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				// TODO: Same apply as child
				cs->next = CoordSys::sq_refobj(v, 3);
				return SQInteger(0);
			}
		);
		pmap[_SC("parent")] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				sq_pushobj(v, cs->parent);
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				// TODO: Same apply as child
				cs->parent = CoordSys::sq_refobj(v, 3);
				return SQInteger(0);
			}
		);
		pmap[_SC("solarSystem")] = PropertyEntry(sqFlagGetter<CS_SOLAR>, sqFlagSetter<CS_SOLAR>);
		pmap[_SC("extent")] = PropertyEntry(sqFlagGetter<CS_EXTENT>, sqFlagSetter<CS_EXTENT>);
		pmap[_SC("isolated")] = PropertyEntry(sqFlagGetter<CS_ISOLATED>, sqFlagSetter<CS_ISOLATED>);
		pmap[_SC("cs_radius")] = PropertyEntry(
			[](HSQUIRRELVM v, const CoordSys *cs){
				sq_pushfloat(v, cs->csrad);
				return SQInteger(1);
			},
			[](HSQUIRRELVM v, CoordSys *cs){
				SQFloat f;
				if(SQ_FAILED(sq_getfloat(v, 3, &f)))
					return sq_throwerror(v, _SC("Cannot convert to float"));
				cs->csrad = f;
				return SQInteger(0);
		});
	}
	return pmap;
}

SQInteger CoordSys::sqf_get(HSQUIRRELVM v){
	CoordSys *p = CoordSys::sq_refobj(v);
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!scstrcmp(wcs, _SC("alive"))){
		sq_pushbool(v, p ? SQTrue : SQFalse);
		return 1;
	}
	else if(!strcmp(wcs, _SC("entlist"))){
		if(!p->w){
			sq_newarray(v, 0); // Returning empty array makes the caller be able to use foreach
			return 1;
		}
		WarField::EntityList &el = p->w->entlist();
		sq_newarray(v, el.size()); // this array
		int idx = 0;
		for(WarField::EntityList::iterator it = el.begin(); it != el.end(); it++) if(*it){
			Entity *e = *it;
			sq_pushinteger(v, idx); // this array idx
			Entity::sq_pushobj(v, e); // this array idx Entity-instance
			sq_set(v, -3); // this array
			idx++;
		}
		return 1;
	}
	else if(!strcmp(wcs, _SC("bulletlist"))){
		if(!p->w){
			sq_newarray(v, 0); // Returning empty array makes the caller be able to use foreach
			return 1;
		}
		WarField::EntityList &bl = p->w->bl;
		sq_newarray(v, bl.size()); // root Entity array
		int idx = 0;
		for(WarField::EntityList::iterator it = bl.begin(); it != bl.end(); it++) if(*it){
			Entity *e = *it;
			sq_pushinteger(v, idx); // root Entity array idx
			Entity::sq_pushobj(v, e); // root Entity array idx instance
			sq_set(v, -3); // root Entity array
			idx++;
		}
		return 1;
	}
	else if(!strcmp(wcs, _SC("extranames"))){
		sq_newarray(v, p->extranames.size());
		for(int i = 0; i < p->extranames.size(); i++){
			sq_pushinteger(v, i);
			sq_pushstring(v, p->extranames[i], -1);
			sq_set(v, -3);
		}
		return 1;
	}
	else if(!strcmp(wcs, _SC("classid"))){
		SQUserPointer typetag;
		sq_gettypetag(v, 1, &typetag);
		sq_pushstring(v, (SQChar*)typetag, -1);
		return 1;
	}
	else if(!scstrcmp(wcs, _SC("rs"))){
		if(!p->w){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("RandomSequencePtr"), -1);
		if(SQ_FAILED(sq_get(v, -2)))
			return sq_throwerror(v, _SC("RandomSequencePtr not defined in root table"));
		sq_createinstance(v, -1);
		sq_setinstanceup(v, -1, &p->w->rs);
		return 1;
	}
	else{
		const CoordSys::PropertyMap &pmap = p->propertyMap();

		CoordSys::PropertyMap::const_iterator it = pmap.find(wcs);
		if(it != pmap.end()){
			if(!it->second.get)
				return sq_throwerror(v, gltestp::dstring(_SC("Property \"")) << wcs << _SC("\" is not readable"));
			return it->second.get(v, p);
		}
		else
			return sq_throwerror(v, gltestp::dstring(_SC("Property \"")) << wcs << _SC("\" not found"));
	}
}

double CoordSys::sqcalc(StellarContext &sc, const char *str, const SQChar *context){
	StackReserver st(sc.v);
	gltestp::dstring dst = gltestp::dstring("return(") << str << ")";
	if(SQ_FAILED(sq_compilebuffer(sc.v, dst, dst.len(), context, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc compile error: " << context);
	sq_pushobject(sc.v, sc.vars);
	if(SQ_FAILED(sq_call(sc.v, 1, SQTrue, SQTrue)))
		throw StellarError(gltestp::dstring() << "sqcalc call error: " << context);
	SQFloat f;
	if(SQ_FAILED(sq_getfloat(sc.v, -1, &f)))
		throw StellarError(gltestp::dstring() << "sqcalc returned value not convertible to float: " << context);
	return f;
}

SQInteger sqf_set(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return sq_throwerror(v, _SC("Invalid _set call"));
	CoordSys *p = CoordSys::sq_refobj(v);
	const SQChar *scs;
	sq_getstring(v, 2, &scs);

	const CoordSys::PropertyMap &pmap = p->propertyMap();

	CoordSys::PropertyMap::const_iterator it = pmap.find(scs);
	if(it != pmap.end()){
		if(!it->second.set)
			return sq_throwerror(v, gltestp::dstring(_SC("Property \"")) << scs << _SC("\" is not writable"));
		return it->second.set(v, p);
	}
	else
		return sq_throwerror(v, gltestp::dstring(_SC("Property \"")) << scs << _SC("\" not found"));
}

SQInteger sqf_cmp(HSQUIRRELVM v){
	if(sq_gettop(v) < 2)
		return SQ_ERROR;
	CoordSys *p = CoordSys::sq_refobj(v);
	CoordSys *o = CoordSys::sq_refobj(v, 2);
	sq_pushinteger(v, p->getid() - o->getid());
	return 1;
}

/// \brief Returns w->accel()
///
/// It should be really a member of WarField or WarSpace, but placed here because the
/// class is assumed as the same as CoordSys from Squirrel scripts' perspective.
static SQInteger sqf_accel(HSQUIRRELVM v){
	CoordSys *p = CoordSys::sq_refobj(v);
	if(!p)
		return sq_throwerror(v, _SC("Object deleted"));

	// It's valid to have no WarField companion object as a CoordSys.  In that case,
	// just return zero vector, which should be appropriate no gravity environment.
	if(!p->w){
		SQVec3d(Vec3d(0,0,0)).push(v);
		return 1;
	}

	// Obtain position and velocity to calculate centrifugal forces (which are
	// responsibility of WarSpace class).
	SQVec3d pos;
	pos.getValue(v, 2);
	SQVec3d velo;
	velo.getValue(v, 3);
	SQVec3d(p->w->accel(pos.value, velo.value)).push(v);
	return 1;
}


bool CoordSys::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, _SC("CoordSys"), -1);
	if(SQ_SUCCEEDED(sq_get(v, -2))){
		sq_poptop(v);
		return false;
	}
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, const_cast<char*>("CoordSys"));
	sq_setclassudsize(v, -1, sq_udsize);
	register_closure(v, _SC("getclass"), sqf_getclass);
	register_closure(v, _SC("getpos"), sqf_getintrinsic2<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::pos>, CoordSys::sq_refobj >);
	register_closure(v, _SC("setpos"), sqf_setintrinsic2<CoordSys, Vec3d, &CoordSys::pos, CoordSys::sq_refobj>);
	register_closure(v, _SC("getvelo"), sqf_getintrinsic2<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::velo>, CoordSys::sq_refobj >);
	register_closure(v, _SC("setvelo"), sqf_setintrinsic2<CoordSys, Vec3d, &CoordSys::velo, CoordSys::sq_refobj>);
	register_closure(v, _SC("getrot"), sqf_getintrinsic2<CoordSys, Quatd, membergetter<CoordSys, Quatd, &CoordSys::rot>, CoordSys::sq_refobj>);
	register_closure(v, _SC("setrot"), sqf_setintrinsic2<CoordSys, Quatd, &CoordSys::rot, CoordSys::sq_refobj>);
	register_closure(v, _SC("getomg"), sqf_getintrinsic2<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::omg>, CoordSys::sq_refobj>);
	register_closure(v, _SC("setomg"), sqf_setintrinsic2<CoordSys, Vec3d, &CoordSys::omg, CoordSys::sq_refobj>);
	register_closure(v, _SC("transPosition"), sqf_transPosition);
	register_closure(v, _SC("transRotation"), sqf_transRotation);
	register_code_func(v, _SC("transVelocity"), _SC("return function(velo, cs){return this.transPosition(velo, cs, true);}"), true);
	register_closure(v, _SC("getpath"), sqf_getpath);
	register_closure(v, _SC("findcs"), sqf_findcs, 2, _SC("xs"));
	register_closure(v, _SC("findcspath"), sqf_findcspath, 2, _SC("xs"));
	register_closure(v, _SC("addent"), sqf_addent, 3, "xsx");
	register_closure(v, _SC("accel"), sqf_accel, 3, "xxx");
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("_set"), sqf_set);
	register_closure(v, _SC("_cmp"), sqf_cmp);
	register_closure(v, _SC("_tostring"), [](HSQUIRRELVM v){
		CoordSys *p = sq_refobj(v, 1);

		// It's not uncommon that a customized Squirrel code accesses a destroyed object.
		if(!p){
			sq_pushstring(v, _SC("(deleted)"), -1);
			return SQInteger(1);
		}

		// Probably we could return CoordSys::name...
		sq_pushstring(v, gltestp::dstring() << "{" << p->classname() << ":" << p->getid() << "}", -1);

		return SQInteger(1);
	}, 1);

	// Property iterator
	register_closure(v, _SC("_nexti"), [](HSQUIRRELVM v){
		CoordSys *p = sq_refobj(v, 1);

		if(!p){
			return sq_throwerror(v, _SC("Iterated CoordSys object is destroyed"));
		}

		auto &pmap = p->propertyMap();
		if(sq_gettype(v, 2) == OT_NULL){
			sq_pushstring(v, pmap.begin()->first, -1);
			return SQInteger(1);
		}
		else{
			const SQChar *prev;
			if(SQ_FAILED(sq_getstring(v, 2, &prev)))
				return sq_throwerror(v, _SC("CoordSys iterator is not a string"));
			auto it = pmap.find(prev);
			if(it == pmap.end())
				return SQInteger(0);
			++it; // Obtain next
			if(it == pmap.end())
				return SQInteger(0);
			sq_pushstring(v, it->first, -1);
			return SQInteger(1);
		}
	});

	sq_createslot(v, -3);
	return true;
}

