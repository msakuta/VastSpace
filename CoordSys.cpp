#define _CRT_SECURE_NO_WARNINGS

#include "CoordSys.h"
#include "astro.h"
#include "Viewer.h"
#include "Player.h"
#include "stellar_file.h"
#include "war.h"
#include "Entity.h"
#include "cmd.h"
#include "serial_util.h"
#include "Respawn.h"
#include "sqadapt.h"
#include "draw/WarDraw.h"
extern "C"{
#include "calc/calc.h"
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
	sc.o << (fullname ? fullname : "");
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
	cpplib::dstring name, fullname;
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

	this->name = strnewdup(name, name.len());
	this->fullname = fullname.len() ? strnewdup(fullname, fullname.len()) : NULL;
	CoordSys *eis = findeisystem();
	if(eis)
		eis->addToDrawList(this);
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

static int findchildv(Vec3d &ret, const CoordSys *retcs, const Vec3d src, const Vec3d srcpos, const CoordSys *cs, const CoordSys *skipcs){
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
		pos = cs2->rot.itrans(v1);

		/* velocity */
		v1 = src - cs2->velo;
		vrot = cs2->omg.vp(pos);
		v1 -= vrot;
		v = v1;
		v = cs2->rot.itrans(v1);

		if(retcs == cs2){
			ret = v;
			return 1;
		}
		if(findchildv(ret, retcs, v, pos, cs2, NULL))
			return 1;
	}
	return 0;
}

static int findparentv(Vec3d &ret, const CoordSys *retcs, const Vec3d src, const Vec3d srcpos, const CoordSys *cs){
	Vec3d v1, v, vrot, pos;

	if(!cs->parent)
		return 0;

	/* velocity */
	v = cs->rot.trans(src);
	v += cs->velo;
	vrot = cs->omg.vp(srcpos);
	v1 = cs->rot.trans(vrot);
	v += v1;

	if(cs->parent == retcs){
		VECCPY(ret, v);
		return 1;
	}

	/* position */
	v1 = cs->rot.trans(srcpos);
	pos = v1 + cs->pos;

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

CoordSys *findchildb(Vec3d &ret, const Vec3d &src, const CoordSys *cs, const CoordSys *skipcs){
	CoordSys *cs2;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
		Vec3d v1, v;
		v1 = src - cs2->pos;
		v = cs2->rot.itrans(v1);
/*		mat4vp3(v, cs2->irot, v1);*/

		/* radius is measured in parent coordinate system and if the position is not within it,
		  we do not belong to this one. */
		if(cs2->belongs(v)){
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

	v1 = cs->rot.trans(src);
/*	MAT4VP3(v1, cs->rot, src);*/
	v = v1 + cs->pos;

	/* if the position vector exceeds sphere's radius, check for parents.
	  otherwise, check for all the children. */
	if(cs->parent->belongs(src)){
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

const CoordSys *CoordSys::belongcs(Vec3d &ret, const Vec3d &src)const{
	CoordSys *csret;
	if(csret = findchildb(ret, src, this, NULL))
		return csret;
	else if(belongs(src))
		return this;
	else
		return findparentb(ret, src, this);
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
	if(p) for(cs = this->children; cs; cs = cs->next) if(strlen(cs->name) == p - path && !strncmp(cs->name, path, p - path)){
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
	bool operator()(CoordSys *&a, CoordSys *&b){
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

void CoordSys::drawWar(war_draw_data *wd){
	;
}

extern double get_timescale();
void CoordSys::anim(double dt){
	Vec3d omgdt;
/*	extern double get_timescale();
	dt *= get_timescale();*/
	omgdt = omg * dt;
	rot = rot.quatrotquat(omgdt);
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next)
		cs->anim(dt);
	if(w)
		w->anim(dt);
}

void CoordSys::postframe(){
	if(w)
		w->postframe();
	CoordSys *cs;
	for(cs = children; cs; cs = cs->next)
		cs->postframe();
}
void CoordSys::endframe(){
	if(w)
		w->endframe();
	vwvalid = 0;
	CoordSys *cs;
	for(cs = children; cs;){
		CoordSys *csnext = cs->next; // cs can unlink itself from the children list
		cs->endframe();
		cs = csnext;
	}

	// delete offsprings first to avoid adoption of children as possible.
	if(flags & CS_DELETE){
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

		// delete WarField if present
		if(w)
			delete w;

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
	char *newname = new char[strlen(name) + 1];
	strcpy(newname, name);
	ret->name = newname;
	ret->fullname = NULL;
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
	if(name)
		delete[] name;
	if(fullname)
		delete[] fullname;
//	delete children;
	// Do not delete siblings, they may be alive after this death.
//	delete next;
	cs_destructs++;
	for(CoordSys *cs = children; cs;){
		CoordSys *csnext = cs->next;
		delete cs;
		cs = csnext;
	}
	delete w;
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

bool CoordSys::readFile(StellarContext &sc, int argc, char *argv[]){
	char *s = argv[0], *ps = argv[1];
	if(!strcmp(s, "name")){
		if(s = ps/*strtok(ps, " \t\r\n")*/){
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
	else if(!strcmp(s, "extraname")){
		if(ps)
			extranames.push_back(ps);
		return true;
	}
	else if(!strcmp(s, "pos")){
		pos[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			pos[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc)
			pos[2] = calc3(&argv[3], sc.vl, NULL);
		return true;
	}
	else if(!strcmp(s, "galactic_coord")){
		double lon = calc3(&argv[1], sc.vl, NULL) / deg_per_rad;
		double lat = 2 < argc ? calc3(&argv[2], sc.vl, NULL) / deg_per_rad : 0.;
		double dist = 3 < argc ? calc3(&argv[3], sc.vl, NULL) : 1e10;
		pos = dist * Vec3d(sin(lon) * cos(lat), cos(lon) * cos(lat), sin(lat));
		return true;
	}
	else if(!strcmp(s, "cs_radius")){
		if(ps)
			csrad = atof(ps);
		return true;
	}
	else if(!strcmp(s, "cs_diameter")){
		if(argv[1])
			csrad = .5 * calc3(&argv[1], sc.vl, NULL);
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
	else if(!strcmp(s, "teleport") || !strcmp(s, "warp")){
		struct teleport *tp;
		const char *name = argc < 2 ? fullname ? fullname : this->name : argv[1];
		if(tp = Player::findTeleport(name)){
			tp->flags |= !strcmp(s, "teleport") ? TELEPORT_TP : TELEPORT_WARP;
			return true;
		}
		tp = Player::addTeleport();
		tp->teleport::teleport(this, name, !strcmp(s, "teleport") ? TELEPORT_TP : TELEPORT_WARP,
			Vec3d(2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0., 3 < argc ? calc3(&argv[3], sc.vl, NULL) : 0., 4 < argc ? calc3(&argv[4], sc.vl, NULL) : 0.));
		return true;
	}
	else if(!strcmp(s, "rstation")){
/*		extern struct player *ppl;
		warf_t *w;
		entity_t *pt;
		if(cs->w)
			w = cs->w;
		else
			w = spacewar_create(cs, ppl);
		pt = RstationNew(w);
		pt->pos[0] = 1 < argc ? calc3(&argv[1], vl, NULL) : 0.;
		pt->pos[1] = 2 < argc ? calc3(&argv[2], vl, NULL) : 0.;
		pt->pos[2] = 3 < argc ? calc3(&argv[3], vl, NULL) : 0.;*/
		return true;
	}
	else if(!strcmp(s, "addent")){
		extern Player *ppl;
		WarField *w;
		Entity *pt;
		if(this->w)
			w = this->w;
		else
			w = this->w = new WarSpace(this)/*spacewar_create(cs, ppl)*/;
		pt = Entity::create(argv[1], w);
		if(pt){
			pt->pos[0] = 2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0.;
			pt->pos[1] = 3 < argc ? calc3(&argv[3], sc.vl, NULL) : 0.;
			pt->pos[2] = 4 < argc ? calc3(&argv[4], sc.vl, NULL) : 0.;
			pt->race = 5 < argc ? atoi(argv[5]) : 0;
		}
		else
			printf("%s(%ld): addent: Unknown entity class name: %s\n", sc.fname, sc.line, argv[1]);
		return true;
	}
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
			rot[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			rot[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc){
			rot[2] = calc3(&argv[3], sc.vl, NULL);
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
			omg[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			omg[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc)
			omg[2] = calc3(&argv[3], sc.vl, NULL);
		if(4 < argc){
			double d;
			omg.normin();
			d = calc3(&argv[4], sc.vl, NULL);
			omg.scalein(d);
		}
		return true;
	}
	else if(!strcmp(s, "updirection")){
		Vec3d v = vec3_000;
		if(1 < argc)
			v[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			v[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc)
			v[2] = calc3(&argv[3], sc.vl, NULL);
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
		HSQUIRRELVM v = g_sqvm;
		StackReserver sr(v);

		// This code is somewhat similar to sqa_console_command.
		try{
			sq_pushroottable(g_sqvm);
			sq_pushstring(g_sqvm, _SC("CoordSys"), -1);
			sq_get(g_sqvm, -2);
			sq_pushstring(g_sqvm, _SC("readFile"), -1);
			sq_get(g_sqvm, -2);
			sq_pushinteger(v, 0);

			if(SQ_FAILED(sq_get(g_sqvm, -2)))
				throw sqa::SQFError(_SC("readFile key is not defined in CoordSys"));
//			sq_pushstring(g_sqvm, s, -1);
			sq_pushroottable(v);
			sq_createinstance(v, -4);
			sqa_newobj(v, this);

			sq_newtable(v);
			for(const varlist *vl = sc.vl; vl; vl = vl->next){
				for(int i = 0; i < vl->c; i++){
					sq_pushstring(v, vl->l[i].name, -1);
					sq_pushfloat(v, vl->l[i].value.d);
					sq_createslot(v, -3);
				}
			}

			// Pass all arguments as strings (no conversion is done beforehand).
			for(int i = 0; i < argc; i++)
				sq_pushstring(v, argv[i], -1);

			// It's no use examining returned value in that case calling the function iteslf fails.
			if(SQ_FAILED(sq_call(v, argc+3, SQTrue, SQTrue)))
				throw sqa::SQFError(_SC("readFile function could not be called"));

			// Assume returned value integer.
			int retint;
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
	return strlen(name) + 1 + ret;
}

cpplib::dstring CoordSys::getpath()const{
	cpplib::dstring ret;
	getpathint(ret);
	return ret;
}

int CoordSys::getpathint(cpplib::dstring &ret)const{
	if(!parent)
		return 0;
	int depth = parent->getpathint(ret);
	ret << '/' << name;
	return depth;
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

static int cmd_ls(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
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

static int cmd_pwd(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	CmdPrint(ppl->cs->getpath());
	return 0;
}

bool CoordSys::registerCommands(Player *ppl){
	CmdAddParam("ls", cmd_ls, ppl);
	CmdAddParam("pwd", cmd_pwd, ppl);
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


/// It's shame
static SQInteger sqf_getclass(HSQUIRRELVM v){
	if(SQ_SUCCEEDED(sq_getclass(v,1)))
		return 1;
	return SQ_ERROR;
}

static SQInteger sqf_addent(HSQUIRRELVM v){
	if(sq_gettop(v) < 3)
		return SQ_ERROR;
	CoordSys *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	
	Vec3d pos = vec3_000;
	if(OT_INSTANCE == sq_gettype(v, -1)){
		SQUserPointer typetag;
		sq_gettypetag(v, -1, &typetag);
		if(typetag == tt_Vec3d){
			Vec3d *pvec;
			sq_pushstring(v, _SC("a"), -1);
			sq_get(v, -2); // this classname vec3 vec3.a
			if(!SQ_FAILED(sq_getuserdata(v, -1, (SQUserPointer*)&pvec, NULL)))
				pos = *pvec;
		}
	}
	sq_pop(v, 2); // this classname

	const SQChar *arg;
	if(SQ_FAILED(sq_getstring(v, 2, &arg)))
		return SQ_ERROR;

	extern Player *ppl;
	WarField *&w = p->w;
	if(!w)
		w = new WarSpace(p)/*spacewar_create(cs, ppl)*/;
	Entity *pt = Entity::create(arg, w);
	if(pt){
		pt->setPosition(&pos);
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
		sq_throwerror(v, cpplib::dstring("addent: Unknown entity class name: %s") << arg);

	sq_pushroottable(v);
	sq_pushstring(v, _SC("Entity"), -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sqa_newobj(v, pt);
	return 1;
}

static SQInteger sqf_name(HSQUIRRELVM v){
	CoordSys *p;
	SQRESULT sr;
	if(!sqa_refobj(v, (SQUserPointer*)&p, &sr))
		return sr;
	sq_pushstring(v, p->name, -1);
	return 1;
}

static SQInteger sqf_child(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!p->children){
		sq_pushnull(v);
		return 1;
	}
/*	int n;
	CoordSys *cs = p->children;
	for(n = 0; cs; n++, cs = cs->next){*/
		sq_pushroottable(v);
		sq_pushstring(v, _SC("CoordSys"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
	sqa_newobj(v, p->children);
//		sq_setinstanceup(v, -1, p->children);
//	}
	return 1;
}

static SQInteger sqf_next(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!p->next){
		sq_pushnull(v);
		return 1;
	}
	sq_pushroottable(v);
	sq_pushstring(v, p->next->getStatic().s_sqclassname, -1);
	sq_get(v, -2);
	sq_createinstance(v, -1);
	sqa_newobj(v, p->next);
//	sq_setinstanceup(v, -1, p->next);
	return 1;
}

static SQInteger sqf_transPosition(HSQUIRRELVM v){
	try{
		CoordSys *p, *p2;
		if(!sqa_refobj(v, (SQUserPointer*)&p))
			return SQ_ERROR;
		if(!sqa_refobj(v, (SQUserPointer*)&p2, NULL, 3))
			return SQ_ERROR;
		SQBool delta;
		if(SQ_FAILED(sq_getbool(v, 4, &delta)))
			delta = SQFalse;
		SQVec3d qv;
		qv.getValue(v, 2);
		qv.value = p->tocs(qv.value, p2, !!delta);
		qv.push(v);
		return 1;
	}
	catch(...){
		return SQ_ERROR;
	}
}

static SQInteger sqf_transRotation(HSQUIRRELVM v){
	try{
		CoordSys *p, *p2;
		if(!sqa_refobj(v, (SQUserPointer*)&p))
			return SQ_ERROR;
		if(!sqa_refobj(v, (SQUserPointer*)&p2, NULL, 2))
			return SQ_ERROR;
		SQQuatd qv(p->tocsq(p2));
		qv.push(v);
		return 1;
	}
	catch(...){
		return SQ_ERROR;
	}
}

static SQInteger sqf_getpath(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	sq_pushstring(v, p->getpath(), -1);
	return 1;
}

static SQInteger sqf_findcspath(HSQUIRRELVM v){
	CoordSys *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	const SQChar *s;
	sq_getstring(v, -1, &s);
	CoordSys *cs = p->findcspath(s);
	if(cs){
		sq_pushroottable(v);
		CoordSys::CtorMap::const_iterator it = CoordSys::ctormap().find(cs->classname());
		if(it == CoordSys::ctormap().end())
			return sq_throwerror(v, _SC("Error wrong classname"));
		sq_pushstring(v, it->second->s_sqclassname, -1);
		sq_get(v, -2);
		if(SQ_FAILED(sq_createinstance(v, -1)))
			return sq_throwerror(v, dstring() << "Couldn't create class " << it->second->s_sqclassname);
		sqa_newobj(v, cs);
//		sq_setinstanceup(v, -1, cs);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

SQInteger CoordSys::sqf_get(HSQUIRRELVM v){
	CoordSys *p;
	const SQChar *wcs;
	sq_getstring(v, 2, &wcs);
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
//	sq_getinstanceup(v, 1, (SQUserPointer*)&p, NULL);
	if(!strcmp(wcs, _SC("entlist"))){
		if(!p->w || !p->w->el){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		sq_pushstring(v, _SC("Entity"), -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, p->w->el);
//		sq_setinstanceup(v, -1, p->w->el);
		return 1;
	}
	else if(!strcmp(wcs, _SC("parentcs"))){
		if(!p->parent){
			sq_pushnull(v);
			return 1;
		}
		sq_pushroottable(v);
		CoordSys::CtorMap::const_iterator it = CoordSys::ctormap().find(p->classname());
		if(it == CoordSys::ctormap().end())
			return sq_throwerror(v, _SC("Error no match class names"));
		sq_pushstring(v, it->second->s_sqclassname, -1);
		sq_get(v, -2);
		sq_createinstance(v, -1);
		sqa_newobj(v, p->parent);
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
	else
		return sqa::sqf_get<CoordSys>(v);
}

bool CoordSys::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, _SC("CoordSys"), -1);
	sq_newclass(v, SQFalse);
	sq_settypetag(v, -1, "CoordSys");
	sq_pushstring(v, _SC("ref"), -1);
	sq_pushnull(v);
	sq_newslot(v, -3, SQFalse);
	register_closure(v, _SC("name"), sqf_name);
	register_closure(v, _SC("getclass"), sqf_getclass);
	register_closure(v, _SC("getpos"), sqf_getintrinsic<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::pos> >);
	register_closure(v, _SC("setpos"), sqf_setintrinsic<CoordSys, Vec3d, &CoordSys::pos>);
	register_closure(v, _SC("getvelo"), sqf_getintrinsic<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::velo> >);
	register_closure(v, _SC("setvelo"), sqf_setintrinsic<CoordSys, Vec3d, &CoordSys::velo>);
	register_closure(v, _SC("getrot"), sqf_getintrinsic<CoordSys, Quatd, membergetter<CoordSys, Quatd, &CoordSys::rot> >);
	register_closure(v, _SC("setrot"), sqf_setintrinsic<CoordSys, Quatd, &CoordSys::rot>);
	register_closure(v, _SC("getomg"), sqf_getintrinsic<CoordSys, Vec3d, membergetter<CoordSys, Vec3d, &CoordSys::omg> >);
	register_closure(v, _SC("setomg"), sqf_setintrinsic<CoordSys, Vec3d, &CoordSys::omg>);
	register_closure(v, _SC("child"), sqf_child);
	register_closure(v, _SC("next"), sqf_next);
	register_closure(v, _SC("transPosition"), sqf_transPosition);
	register_closure(v, _SC("transRotation"), sqf_transRotation);
	register_code_func(v, _SC("transVelocity"), _SC("return function(velo, cs){return this.transPosition(velo, cs, true);}"), true);
	register_closure(v, _SC("getpath"), sqf_getpath);
	register_closure(v, _SC("findcspath"), sqf_findcspath, 2, _SC("xs"));
	register_closure(v, _SC("addent"), sqf_addent, 3, "xsx");
	register_closure(v, _SC("_get"), sqf_get);
	register_closure(v, _SC("_set"), sqf_set<CoordSys>);
	sq_pushstring(v, _SC("readFile"), -1);
	sq_newarray(v, 1);
	sq_newslot(v, -3, SQFalse); // The last argument is important to designate the readFile handler is static.
	sq_createslot(v, -3);
	return true;
}

