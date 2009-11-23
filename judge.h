/* 3-Dimentional hit judgement routines */
#ifndef JUDGE_H
#define JUDGE_H
#include "entity.h"
#include <cpplib/quat.h>
#include <cpplib/vec3.h>

extern int jHitSphere(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt);
extern int jHitSpherePos(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *f, Vec3d *ret);
extern int jHitPolygon(const double vertex_buffer[][3], unsigned short vertex_indices[], int vertex_count, const double src[3], const double dir[3], double mint, double maxt, double *ret_param, double (*ret_pos)[3], double (*ret_normal)[3]);
extern int jHitBox(const Vec3d &org, const Vec3d &scale, const Quatd &rot, const Vec3d &src, const Vec3d &dir, double mint, double maxt, double *ret, Vec3d *retp, Vec3d *retn);

struct hitbox{
	Vec3d org;
	Quatd rot;
	Vec3d sc;
};

/* Object tree */
union unode;
struct otnt{
	avec3_t pos;
	double rad;
	int leaf;
	union unode *a[2];
};

union unode{Entity *t; otnt n;};
#if 0
entity_t *otjHitSphere(const otnt *root, const avec3_t *src, const avec3_t *dir, double dt, double rad, avec3_t *pos);

/* otjHitSphere doesn't check all overwrapping spheres but one of them, so this function is here.
  Because it takes so many arguments, they are passed as in form of pointer to structure.
  For internal recursive call, this form is expected to save stack usage too. */
#define OTJ_CALLBACK 0x1
#define OTJ_IGLIST 0x2
#define OTJ_IGVFT 0x4
struct otjEnumHitSphereParam{
	const otnt *root;
	const avec3_t *src;
	const avec3_t *dir;
	double dt;
	double rad;
	avec3_t *pos;
	avec3_t *norm;
	unsigned flags; /* Following optional functionailties are designated as being used by ORing corresponding flags. */
	int (*callback)(const struct otjEnumHitSphereParam *, entity_t *pt);
	void *hint;
	entity_t **iglist; /* ignore list; matching entries are not checked, faster than to check in callback */
	int niglist; /* count of iglist */
	struct entity_static **igvft; /* ignore vft list; vft version of iglist */
	int nigvft; /* count of igvft */
};
entity_t *otjEnumHitSphere(const struct otjEnumHitSphereParam *);
entity_t *otjEnumPointHitSphere(const struct otjEnumHitSphereParam *);
entity_t *otjEnumNearestPoint(const struct otjEnumHitSphereParam *);
entity_t *otjEnumNearestSphere(const struct otjEnumHitSphereParam *);

otnt *ot_build(warf_t *w, double dt);
void ot_draw(warf_t *w, wardraw_t *wd);
#endif
#endif
