/* 3-Dimentional hit judgement routines */
#ifndef JUDGE_H
#define JUDGE_H
#include "entity.h"
#include <cpplib/quat.h>
#include <cpplib/vec3.h>
#include <vector>

extern bool jHitSphere(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt);
extern bool jHitSpherePos(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *f, Vec3d *ret);
extern int jHitPolygon(const double vertex_buffer[][3], unsigned short vertex_indices[], int vertex_count, const double src[3], const double dir[3], double mint, double maxt, double *ret_param, double (*ret_pos)[3], double (*ret_normal)[3]);
extern bool jHitBox(const Vec3d &org, const Vec3d &scale, const Quatd &rot, const Vec3d &src, const Vec3d &dir, double mint, double maxt, double *ret = NULL, Vec3d *retp = NULL, Vec3d *retn = NULL);
double jHitTriangle(const Vec3d &b, const Vec3d &c, const Vec3d &org, const Vec3d &end);
int jHitLines(const Vec3d &apos, const Quatd &arot, const Vec3d &avelo, const Vec3d &aomg, double alen, double blen, double dt);

// Axis Aligned Bounding Box
struct AABB{
	Vec3d p0, p1;
	AABB(const Vec3d &a0 = Vec3d(0,0,0), const Vec3d &a1 = Vec3d(0,0,0)) : p0(a0), p1(a1){}
	void unite(const Vec3d &v){
		for(int i = 0; i < 3; i++){
			if(v[i] < p0[i])
				p0[i] = v[i];
			if(p1[i] < v[i])
				p1[i] = v[i];
		}
	}
	void unite(const AABB &o){
		unite(o.p0);
		unite(o.p1);
	}
	bool includes(const Vec3d &v)const{
		return p0[0] < v[0] && v[0] < p1[0] && p0[1] < v[1] && v[1] < p1[1] && p0[2] < v[2] && v[2] < p1[2];
	}
};

struct hitbox{
	Vec3d org;
	Quatd rot;
	Vec3d sc;
	hitbox(const Vec3d &aorg = Vec3d(0,0,0), const Quatd &arot = Quatd(0,0,0,1), const Vec3d &asc = Vec3d(1,1,1)) : org(aorg), rot(arot), sc(asc){}
	hitbox(const AABB &aabb) : org((aabb.p0 + aabb.p1) / 2.), sc((aabb.p1 - aabb.p0) / 2.), rot(Quatd(0,0,0,0)){}
	operator const AABB()const;
};

struct contact_info{
	Vec3d pos; // Position
	Vec3d normal; // Normal
	Vec3d velo; /* base velocity of colliding object */
	double depth; /* penetration depth */
};

class Shape{
public:
	virtual const char *id()const = 0;
	virtual bool derived(const char *classid)const;
	virtual bool intersects(const Shape &o, const Entity &se, const Entity &oe, contact_info *ci = NULL)const = 0;
	virtual bool boundingBox(AABB &)const = 0;
};

class BoxShape : public Shape{
public:
	static const char *sid;
	virtual const char *id()const;
	virtual bool derived(const char *classid)const;
	hitbox hb;
	virtual bool intersects(const Shape &o, const Entity &se, const Entity &oe, contact_info *ci = NULL)const;
	virtual bool boundingBox(AABB &)const;
};

class CompoundShape : public Shape{
public:
	static const char *sid;
	virtual const char *id()const;
	virtual bool derived(const char *classid)const;
	AABB aabb;
	std::vector<Shape*> comp;
	virtual bool intersects(const Shape &o, const Entity &se, const Entity &oe, contact_info *ci = NULL)const;
	virtual bool boundingBox(AABB &)const;
	void recalcBB();
};

extern int jHitBoxPlane(const hitbox &hb, const Vec3d &planeorg, const Vec3d &planenorm);

/* Object tree */
union unode;
struct otnt{
	Vec3d pos;
	double rad;
	int leaf;
	union unode{
		Entity *t;
		otnt *n;
	} a[2];
};

//union unode{Entity *t; otnt n;};
#if 1
Entity *otjHitSphere(const otnt *root, const Vec3d &src, const Vec3d &dir, double dt, double rad, Vec3d *pos);

/* otjHitSphere doesn't check all overwrapping spheres but one of them, so this function is here.
  Because it takes so many arguments, they are passed as in form of pointer to structure.
  For internal recursive call, this form is expected to save stack usage too. */
#define OTJ_CALLBACK 0x1
#define OTJ_IGLIST 0x2
#define OTJ_IGVFT 0x4
struct otjEnumHitSphereParam{
	const otnt *root;
	const Vec3d *src;
	const Vec3d *dir;
	double dt;
	double rad;
	Vec3d *pos;
	Vec3d *norm;
	unsigned flags; /* Following optional functionailties are designated as being used by ORing corresponding flags. */
	int (*callback)(const struct otjEnumHitSphereParam *, Entity *pt);
	void *hint;
	Entity **iglist; /* ignore list; matching entries are not checked, faster than to check in callback */
	int niglist; /* count of iglist */
//	struct entity_static **igvft; /* ignore vft list; vft version of iglist */
//	int nigvft; /* count of igvft */
};
Entity *otjEnumHitSphere(const struct otjEnumHitSphereParam *);
Entity *otjEnumPointHitSphere(const struct otjEnumHitSphereParam *);
Entity *otjEnumNearestPoint(const struct otjEnumHitSphereParam *);
Entity *otjEnumNearestSphere(const struct otjEnumHitSphereParam *);

otnt *ot_build(WarSpace *w, double dt);
otnt *ot_check(WarSpace *w, double dt);
void ot_draw(WarSpace *w, wardraw_t *wd);
#endif
#endif
