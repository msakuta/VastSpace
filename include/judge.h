/* 3-Dimentional hit judgement routines */
#ifndef JUDGE_H
#define JUDGE_H
#include "export.h"
#include "Entity.h"
#include <cpplib/quat.h>
#include <cpplib/vec3.h>

EXPORT extern bool jHitSphere(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt);
EXPORT extern bool jHitSpherePos(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *f, Vec3d *ret);
EXPORT extern int jHitPolygon(const double vertex_buffer[][3], unsigned short vertex_indices[], int vertex_count, const double src[3], const double dir[3], double mint, double maxt, double *ret_param, double (*ret_pos)[3], double (*ret_normal)[3]);
EXPORT extern bool jHitBox(const Vec3d &org, const Vec3d &scale, const Quatd &rot, const Vec3d &src, const Vec3d &dir, double mint, double maxt, double *ret, Vec3d *retp, Vec3d *retn);
EXPORT double jHitTriangle(const Vec3d &b, const Vec3d &c, const Vec3d &org, const Vec3d &end, double *bcoord = NULL, double *ccoord = NULL);
EXPORT int jHitLines(const Vec3d &apos, const Quatd &arot, const Vec3d &avelo, const Vec3d &aomg, double alen, double blen, double dt, double *hitt = NULL);

struct hitbox{
	Vec3d org;
	Quatd rot;
	Vec3d sc;
	hitbox(Vec3d aorg = Vec3d(0,0,0), Quatd arot = Quatd(0,0,0,1), Vec3d asc = Vec3d(0,0,0)) : org(aorg), rot(arot), sc(asc){}
};

EXPORT extern int jHitBoxPlane(const hitbox &hb, const Vec3d &planeorg, const Vec3d &planenorm);
EXPORT extern int jHitBoxes(const hitbox &hb1, const hitbox &hb2, const Vec3d &rvelo, const Vec3d &romg, double dt, double *hitt = NULL);

/* Object tree */
union unode;
struct otnt{
	Vec3d pos;
	double rad;
	int leaf;
	/// We want this member to be a union, but we cannot because
	/// it contains an object that needs constructor invocation to initialize.
	struct unode{
		WeakPtr<Entity> t;
		otnt *n;
	} a[2];
};

//union unode{Entity *t; otnt n;};
#if 1
EXPORT Entity *otjHitSphere(const otnt *root, const Vec3d &src, const Vec3d &dir, double dt, double rad, Vec3d *pos);

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
EXPORT Entity *otjEnumHitSphere(const struct otjEnumHitSphereParam *);
EXPORT Entity *otjEnumPointHitSphere(const struct otjEnumHitSphereParam *);
EXPORT Entity *otjEnumNearestPoint(const struct otjEnumHitSphereParam *);
EXPORT Entity *otjEnumNearestSphere(const struct otjEnumHitSphereParam *);

EXPORT otnt *ot_build(WarSpace *w, double dt);
EXPORT otnt *ot_check(WarSpace *w, double dt);
EXPORT void ot_draw(WarSpace *w, wardraw_t *wd);
#endif
#endif
