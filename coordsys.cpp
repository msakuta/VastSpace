#define _CRT_SECURE_NO_WARNINGS

#include "coordsys.h"
#include "astro.h"
#include "viewer.h"
#include "player.h"
#include "stellar_file.h"
#include "war.h"
#include "entity.h"
#include "cmd.h"
#include "serial_util.h"
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
#include <algorithm>
#include <string>
#include <iomanip>

#define numof(a) (sizeof(a)/sizeof*(a))

long tocs_invokes = 0;
long tocs_parent_invokes = 0;
long tocs_children_invokes = 0;

const char *CoordSys::classname()const{
	return "CoordSys";
}


void CoordSys::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << " (" << name << ") (" << (fullname ? fullname : "") << ") " << sc.map[parent] << " " << sc.map[children] << " " << sc.map[next] << " " << pos << " " << velo << " " << qrot;
	sc.o << " " << omg;
	sc.o << " " << csrad;
	sc.o << " " << flags;
}

void CoordSys::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	cpplib::dstring name, fullname;
	unsigned parent, children, next;
	sc.i >> " (";
	name = readUntil(sc.i, ')');
	sc.i >> ") (";
	fullname = readUntil(sc.i, ')');
	sc.i >> ") ";
	sc.i >> parent >> " " >> children >> " " >> next >> " " >> pos >> " " >> velo >> " " >> qrot;
	sc.i >> " " >> omg;
	sc.i >> " " >> csrad;
	sc.i >> " " >> flags;

	this->name = strnewdup(name, name.len());
	this->fullname = fullname.len() ? strnewdup(fullname, fullname.len()) : NULL;
	this->parent = static_cast<CoordSys*>(sc.map[parent]);
	this->children = static_cast<CoordSys*>(sc.map[children]);
	this->next = static_cast<CoordSys*>(sc.map[next]);
	CoordSys *eis = findeisystem();
	if(eis)
		eis->addToDrawList(this);
}

const unsigned CoordSys::classid = registerClass("CoordSys", Conster<CoordSys>);

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

void CoordSys::adopt_child(CoordSys *newparent, bool retain){
	CoordSys *adoptee = this;
	CoordSys **ppcs2;
	Vec3d pos, velo;
	Quatd q;

	/* already adopted */
	if(adoptee->parent == newparent)
		return;

	/* obtain relative rotation matrix between old node to new node. */
	if(retain){
		q = adoptee->tocsq(newparent);

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
		adoptee->qrot = q;
	}

	/* recognize the new parent */
	adoptee->parent = newparent;

	/* append the child into new family list */
	adoptee->next = newparent->children;
	newparent->children = adoptee;
}



static int findchild(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, const CoordSys *skipcs, bool delta){
	CoordSys *cs2;
	tocs_children_invokes++;
	for(cs2 = cs->children; cs2; cs2 = cs2->next) if(cs2 != skipcs)
	{
		Vec3d v1, v;
		if(delta){
			v = cs2->qrot.itrans(src);
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

static int findparent(Vec3d &ret, const CoordSys *retcs, const Vec3d &src, const CoordSys *cs, bool delta){
	Vec3d v1, v;
	tocs_parent_invokes++;

	if(!cs->parent)
		return 0;

	if(delta){
		v = cs->qrot.trans(src);
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
		pos = cs2->qrot.itrans(v1);

		/* velocity */
		v1 = src - cs2->velo;
		vrot = cs2->omg.vp(pos);
		v1 -= vrot;
		v = v1;
		v = cs2->qrot.itrans(v1);

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
	v = cs->qrot.trans(src);
	v += cs->velo;
	vrot = cs->omg.vp(srcpos);
	v1 = cs->qrot.trans(vrot);
	v += v1;

	if(cs->parent == retcs){
		VECCPY(ret, v);
		return 1;
	}

	/* position */
	v1 = cs->qrot.trans(srcpos);
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
		v = cs2->qrot.itrans(v1);
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

	v1 = cs->qrot.trans(src);
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

void CoordSys::predraw(const Viewer *vw){
	for(CoordSys *cs = children; cs; cs = cs->next)
		cs->predraw(vw);
}

void CoordSys::csMap(std::map<Serializable *, unsigned> &cm){
	if(cm.find(this) == cm.end()){
		unsigned id = cm.size();
		cm[this] = id;
	}
	for(CoordSys *cs = children; cs; cs = cs->next)
		cs->csMap(cm);
}

void CoordSys::csSerialize(SerializeContext &sc){
	serialize(sc);
	sc.o << std::endl;
	for(CoordSys *cs = children; cs; cs = cs->next)
		cs->csSerialize(sc);
}

void CoordSys::csUnmap(UnserializeContext &sc){
	while(!sc.i.eof()){
		std::string line;
		getline(sc.i, line);
		if(line.length() == 0)
			break;
		size_t space = line.find(' ');
		std::string cname = space != line.npos ? line.substr(0, space) : line;
		if(sc.cons[cname]){
			sc.map.push_back(sc.cons[cname]());
		}
	}
/*	for(CoordSys *cs = children; cs; cs = cs->next)
		cs->csUnmap(sc);*/
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

void CoordSys::drawcs(const Viewer *vw){
	draw(vw);
	Vec3d cspos;
	cspos = vw->cs->tocs(avec3_000, this);
/*	if(cs->flags & CS_EXTENT && vw->gc && glcullFrustum(cspos, cs->rad, vw->gc))
		return;*/
	if((CS_EXTENT | CS_ISOLATED) == (flags & (CS_EXTENT | CS_ISOLATED))){
/*		{
			timemeas_t tm;
			TimeMeasStart(&tm);*/
		std::sort(aorder.begin(), aorder.end(), AstroCmp(*vw));
/*			printf("sortcs %d %lg\n", n, TimeMeasLap(&tm));
		}*/
		AOList::reverse_iterator i = aorder.rbegin();
		for(; i != aorder.rend();i++) if(*i){
			if(*i == this)
				(*i)->draw(vw);
			else
				(*i)->drawcs(vw);
		}
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
	qrot = qrot.quatrotquat(omgdt);
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
		if(parent) for(CoordSys **ppcs = &parent->children; *ppcs; ppcs = &(*ppcs)->next) if(*ppcs == this){
			*ppcs = next;
			break;
		}

		// give children to parent.
		for(cs = children; cs;){
			CoordSys *csnext = cs->next;
			cs->velo = parent->tocsv(cs->velo, cs->pos, this);
			cs->pos = parent->tocs(cs->pos, this);
			cs->omg = parent->tocs(cs->omg, this, true);
			cs->qrot = parent->tocsq(this);
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
	VECNULL(ret->pos);
	VECNULL(ret->velo);
	QUATIDENTITY(ret->qrot);
	VECNULL(ret->omg);
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
	delete children;
	delete next;
	cs_destructs++;
/*	for(CoordSys *cs = children; cs;){
		CoordSys *csnext = cs->next;
		delete cs;
		cs = csnext;
	}*/
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
	else if(!strcmp(s, "pos")){
		pos[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			pos[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc)
			pos[2] = calc3(&argv[3], sc.vl, NULL);
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
		int i;
		for(i = 0; i < ntplist; i++) if(!strcmp(tplist[i].name, name)){
			tplist[i].flags |= !strcmp(s, "teleport") ? TELEPORT_TP : TELEPORT_WARP;
			break;
		}
		if(i != ntplist)
			return true;
		tplist = (teleport*)realloc(tplist, ++ntplist * sizeof *tplist);
		tp = &tplist[ntplist-1];
		tp->cs = this;
		tp->name = (char*)malloc(strlen(name) + 1);
		strcpy(tp->name, name);
		tp->flags = !strcmp(s, "teleport") ? TELEPORT_TP : TELEPORT_WARP;
		tp->pos[0] = 2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0.;
		tp->pos[1] = 3 < argc ? calc3(&argv[3], sc.vl, NULL) : 0.;
		tp->pos[2] = 4 < argc ? calc3(&argv[4], sc.vl, NULL) : 0.;
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
			w = this->w = new WarField(this)/*spacewar_create(cs, ppl)*/;
		pt = Entity::create(argv[1], w);
		if(pt){
			pt->pos[0] = 2 < argc ? calc3(&argv[2], sc.vl, NULL) : 0.;
			pt->pos[1] = 3 < argc ? calc3(&argv[3], sc.vl, NULL) : 0.;
			pt->pos[2] = 4 < argc ? calc3(&argv[4], sc.vl, NULL) : 0.;
			pt->race = 5 < argc ? atoi(argv[5]) : 0;
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
		if(1 < argc)
			qrot[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			qrot[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc){
			qrot[2] = calc3(&argv[3], sc.vl, NULL);
			qrot[3] = sqrt(1. - VECSLEN(qrot));
		}
		return true;
	}
	else if(!strcmp(s, "omega")){
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
		avec3_t v = {0};
		if(1 < argc)
			v[0] = calc3(&argv[1], sc.vl, NULL);
		if(2 < argc)
			v[1] = calc3(&argv[2], sc.vl, NULL);
		if(3 < argc)
			v[2] = calc3(&argv[3], sc.vl, NULL);
		quatdirection(qrot, v);
		return true;
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
			CmdPrintf(ds);
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
		CmdPrintf("Could not find path %s", argv[1]);
	return 0;
}

static int cmd_pwd(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	char buf[128];
	ppl->cs->getpath(buf, sizeof buf);
	CmdPrintf(buf);
	return 0;
}

bool CoordSys::registerCommands(Player *ppl){
	CmdAddParam("ls", cmd_ls, ppl);
	CmdAddParam("pwd", cmd_pwd, ppl);
	return true;
}

