#include "judge.h"
#include "astro.h"
#include "Viewer.h"
#include "draw/WarDraw.h"
extern "C"{
#include <clib/timemeas.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
#include <clib/stats.h>
}
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>

bool jHitSphere(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt){
	return jHitSpherePos(obj, radius, src, dir, dt, NULL, NULL);
}

/* determine intersection of a sphere shell and a ray. it's equivalent to inter-sphere hit detection,
  when the radius argument is sum of the two spheres. */
bool jHitSpherePos(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *retf, Vec3d *pos, double *dist){
	double b, c, D, d, t0, t1, dirslen;
	bool ret;

	Vec3d del = src - obj;

	/* scalar product of the ray and the vector. */
	b = dir.sp(del);

	/* ??? */
	dirslen = dir.slen();
	c = dirslen * (del.slen() - radius * radius);

	// We can always return the distance of the ray and the sphere's center.
	if(dist){
		*dist = (del - b / dirslen * dir).len();
		// Return the nearest point to the sphere's center on the line if no hit detected.
		if(pos)
			*pos = del - b / dirslen * dir;
	}

	/* discriminant?? */
	D = b * b - c;
	if(D <= 0)
		return false;

	d = sqrt(D);

	/* we need vector equation's parameter value to determine hitness with line segment */
	if(dirslen == 0.)
		return false;
	t0 = (-b - d) / dirslen;
	t1 = (-b + d) / dirslen;

	ret = 0. <= t1 && /*t0 < dt || 0. <= t1 &&*/ t0 < dt;

	if(ret && (pos || retf)){
		if(t0 < 0 /*&& dt < t1*/){
			if(pos)
				(*pos).clear();
			if(retf)
				*retf = 0.;
		}
/*		else if(t0 < 0)
			VECSCALE(pos, dir, t1);*/
		else /*if(dt <= t1)*/{
			if(pos)
				*pos = dir * t0;
			if(retf)
				*retf = t0;
		}
		if(pos)
			*pos += src;
	}

	return ret;
}

/* intersection of a ray and coplanar, convex and not self-crossing polygon with arbitrary
  number of vertices. returns parameter value (t of src + t * dir) to object pointed to
  by ret on hit. it doesn't check back face. */
int jHitPolygon(const double vb[][3], unsigned short vi[], int nv, const double src[3], const double dir[3], double mint, double maxt, double *ret, double (*retp)[3], avec3_t *retn){
	double t, dn, sa[3];
	double n[3], p[3];
	int i;
	if(nv < 3) return 0; /* have no area */

	/* determine normal vector by obtaining vector product of difference vectors on the plane. */
	{
		double v01[3], v02[3];
		VECSUB(v01, vb[vi[1]], vb[vi[0]]);
		VECSUB(v02, vb[vi[2]], vb[vi[0]]);
		VECVP(n, v01, v02);
	}

	/* the line and the plane are parallel or piercing from back */
	if((dn = VECSP(dir, n)) <= 0.)
		return 0;

	/* we can normalize here to avoid costly sqrt in parallel or back-piercing ray. */
	{
		double nl;
		nl = VECLEN(n);
		dn /= nl;
		VECSCALEIN(n, 1. / nl);
	}

	/* calculate vector parameter */
	VECSUB(sa, src, vb[vi[0]]);
	t = -VECSP(sa, n) / dn;
	if(t < mint || maxt < t)
		return 0;

	/* find intersecting point */
	VECCPY(p, src);
	VECSADD(p, dir, t);

	/* for all polygon edges, determine left-handedness of intersecting point,
	  which indicates the point is inside the polygon loop. because of the
	  algorithm, you cannot correctly judge hitness if it is non-coplanar,
	  non-convex or self-crossing. */
	for(i = 0; i < nv; i++){
		const double *v0 = vb[vi[i]], *v1 = vb[vi[(i+1) % nv]];
		double v0p[3], v01[3], vp[3];
		VECSUB(v0p, v0, p);
		VECSUB(v01, v0, v1);
		VECVP(vp, v01, v0p);
		if(VECSP(vp, n) < 0)
			return 0;
	}

	/* if all checks are passed, it's intersecting. */
	if(ret) *ret = t;
	if(retp) VECCPY(*retp, p);
	if(retn) VECCPY(*retn, n);
	return 1;
}

// Trace a ray whether it intersects a hitbox.
bool jHitBox(const Vec3d &org, const Vec3d &scale, const Quatd &rot, const Vec3d &src, const Vec3d &dir, double mint, double maxt, double *ret, Vec3d *retp, Vec3d *retn){
	int i;
	bool reti = false;
	Mat4d imat, mat;
	Vec3d lsrc, ldir;

	imat = rot.cnj().tomat4();
/*	for(i = 0; i < 3; i++)
		VECSCALEIN(&imat[i*4], 1. / scale[i]);*/
/*	MAT4SCALE(imat, 1. / scale[0], 1. / scale[1], 1. / scale[2]);*/

	mat = rot.tomat4();
/*	MAT4SCALE(mat, scale[0], scale[1], scale[2]);*/
/*	{
		amat3_t mat3, imat3;
		MAT4TO3(mat3, mat);
		matinv(imat3, mat3);
		MAT3TO4(imat, imat3);
	}*/
	mat.vec3(3) += org;

/*	MAT4TRANSLATE(imat, -org[0], -org[1], -org[2]);*/
/*	VECSCALE(&imat[12], org, -1);*/

	{
		Vec3d dr;
		dr = src - org;
		lsrc = imat.dvp3(dr);
		ldir = imat.dvp3(dir);
	}

	if(ldir[0] == 0 && ldir[1] == 0 && ldir[2] == 0 || fabs(lsrc[0]) < scale[0] && fabs(lsrc[1]) < scale[1] && fabs(lsrc[2]) < scale[2]){ // point hit; avoid zero division
		double dists[3], bestdist = HUGE_VAL;
		int besti = -1;
		for(i = 0; i < 3; i++){
			dists[i] = scale[i] - fabs(lsrc[i]);
			if(dists[i] < 0)
				return 0;
			else if(dists[i] < bestdist){
				bestdist = dists[i];
				besti = i;
			}
		}
		if(0 <= besti){
			if(ret) *ret = mint;
			if(retp) *retp = src;
			if(retn) *retn = lsrc[besti] / fabs(lsrc[besti]) * mat.vec3(besti).norm();
			reti = true;
		}
	}
	else{
		double f, best = maxt;
		Vec3d hit;
		for(i = 0; i < 3; i++){
			int s = ldir[i] < 0. ? -1 : 1, a = i;
			if(ldir[a] == 0.) // Avoid zerodiv
				continue;
			f = (-lsrc[a] - s * scale[a]) / ldir[a];
			if(!(mint <= f && f < best))
				continue;
			hit = ldir * f;
			hit += lsrc;
			if(-scale[(a+1)%3] < hit[(a+1)%3] && hit[(a+1)%3] < scale[(a+1)%3] && -scale[(a+2)%3] < hit[(a+2)%3] && hit[(a+2)%3] < scale[(a+2)%3]){
				if(ret) *ret = f;
				if(retp){
/*					mat4vp3(*retp, mat, hit);*/
					*retp = dir * f;
					*retp += src;
				}
				if(retn){
/*					quatrot(*retn, rot, a == 0 ? avec3_100 : a == 1 ? avec3_010 : avec3_001);*/
					*retn = -s * mat.vec3(a).norm();
				}
				best = f;
				reti = true;
			}
		}
	}
	return reti;
}

// Tests whether a hitbox is intersecting with a plane.
int jHitBoxPlane(const hitbox &hb, const Vec3d &planeorg, const Vec3d &planenorm){
	int ret = 0;
	for(int ix = 0; ix < 2; ix++) for(int iy = 0; iy < 2; iy++) for(int iz = 0; iz < 2; iz++){

		// Obtain a corner's position in hitbox'es local coordinates.
		Vec3d org = Vec3d((ix * 2 - 1) * hb.sc[0], (iy * 2 - 1) * hb.sc[1], (iz * 2 - 1) * hb.sc[2]);
		org = hb.rot.trans(org);
		Vec3d worldpos = org + hb.org;
		ret |= ((worldpos - planeorg).sp(planenorm) < 0.) << (ix * 4 + iy * 2 + iz);
	}
	return ret;
}

/// \brief An argument mapping function that calls jHitCylinderPos with some default values.
///
/// Probably we no longer need this sort of thing, since a C++ class Vec3d is already used in the argument list,
/// so we cannot support C in the first place, thus default values can be used without worrying about breaking compatibility.
bool jHitCylinder(const Vec3d &org, const Vec3d &axis, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *retf){
	return jHitCylinderPos(org, axis, radius, src, dir, dt, retf, NULL, NULL);
}


static Vec3d projectPlane(const Vec3d &axis, const Vec3d &v){
	return v - v.sp(axis) * axis;
}

/// \brief Checks and returns hit position among a ray and a cylinder.
///
/// This function is under construction; it may compile, but probably won't work correctly.
bool jHitCylinderPos(const Vec3d &org, const Vec3d &axis, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *retf, Vec3d *pos, double *dist){

	// Ignore the case that the axis vector is invalid.
	if(axis.slen() <= 0.){
		assert(0 < axis.slen()); // This case seems like a bug.
		return false;
	}

	// Convert the source position of the ray to the cylinder's local coordinates.
	Vec3d del = src - org;

	Vec3d naxis = axis.norm(); // Normalized axis

	// The ray vector projected on the planes at the both ends of the cylinder.
	Vec3d plray = del - naxis * del.sp(naxis);

	/* scalar product of the ray and the vector. */
	double b = dir.sp(plray);

	/* ??? */
	double dirslen = plray.slen();
	double c = dirslen * (plray.slen() - radius * radius);

	// We can always return the distance of the ray and the sphere's center.
	if(dist){
		*dist = (plray - b / dirslen * plray).len();
		// Return the nearest point to the sphere's center on the line if no hit detected.
		if(pos)
			*pos = plray - b / dirslen * plray;
	}

	/* discriminant?? */
	double D = b * b - c;
	// If the ray won't hit the cylinder extended infinitely along the axis,
	// it won't hit any part of the finite cylinder.
	// It's no use checking planes at both ends of the cylinder, so we exit here.
	if(D <= 0)
		return false;

	double d = sqrt(D);

	/* we need vector equation's parameter value to determine hitness with line segment */
	if(dirslen == 0.)
		return false;
	double t0 = (-b - d) / dirslen; // The 'earlier' solution
	double t1 = (-b + d) / dirslen; // The 'later' solution

	bool buried = false;

	double t = dt;

	// This block checks t0, but if both of intersecting points are behind the ray's line segment,
	// we've already gone through the cylinder's side (but not necessarily the top and the bottom),
	// so we will ignore the case.
	if(0. < t1){
		// If the ray's starting point is buried inside the cylinder's volume,
		// consider the point is the starting point.
		if(t0 < 0.)
			t0 = 0.;
		// If the earlier intersecting point is within cylinder's length along the axis,
		// consider it hits.
		if(t0 < dt && fabs(naxis.sp(dir * t0 + del)) < axis.len())
			t = t0, buried = true;
	}

	// Same goes for later intersecting point.
//	if(0 <= t1 && t1 < t && fabs(naxis.sp(dir * t1 + del)) < axis.len())
//		t = t1, ret = 2;

	// Dot product of dir and axis. It can be easily zero.
	double dir_dot_axis = dir.sp(axis);

	if(!buried && dir_dot_axis != 0.){
		// Time parameter values for points intersecting positive and negative planes.
		double tp = (del - axis).sp(axis) / dir_dot_axis;
		double tn = (del + axis).sp(axis) / dir_dot_axis;

		// Distances from the cylinder axis.
		double distp = projectPlane(naxis, plray * tp).len();
		double distn = projectPlane(naxis, plray * tn).len();

		if(distp < radius){
			if(0 <= tp && tp < t)
				t = tp;
		}

		if(distn < radius){
			if(0 <= tn && tn < t)
				t = tn;
		}
	}

	if(t < dt && (pos || retf)){
		// If the ray's starting point is buried inside the cylinder's volume,
		// return time parameter value 0.
		{
			if(pos)
				*pos = dir * t;
			if(retf)
				*retf = t;
		}
		if(pos)
			*pos += src;
	}

	return t != dt;
}


// Intersection of triangle(O,a,b) and line segment(org,end)
double jHitTriangle(const Vec3d &b, const Vec3d &c, const Vec3d &org, const Vec3d &end, double *bcoord, double *ccoord){
	// Acquire the third axis (= normal of the triangle plane)
	Vec3d bvpc = b.vp(c)/*.norm()*/;

//	Vec3d end = org + Vec3d(0,0,1);

	// Obtain local z position of start and end point
	double orgz = org.sp(bvpc);
	double endz = end.sp(bvpc);

	// If starting and ending point of the line lies the same side of the triangle plane, there's no way intersecting.
	if(0. < orgz * endz)
		return 0;

	Vec3d caxis = bvpc.vp(b);
	Vec3d baxis = c.vp(bvpc);
	caxis /= caxis.sp(c);
	baxis /= baxis.sp(b);

	// Transform to local
	Vec3d lpos(org.sp(baxis), org.sp(caxis), orgz);
	Vec3d ldir = Vec3d(end.sp(baxis), end.sp(caxis), endz) - lpos;

	// Find intersecting point
	double f = -lpos[2] / (endz - lpos[2]);
	Vec3d lhit = lpos + f * ldir;
	bool ret = 0. < lhit[0] && 0. < lhit[1] && lhit[0] + lhit[1] < 1.;
	if(ret){
		if(bcoord) *bcoord = lhit[0];
		if(ccoord) *ccoord = lhit[1];
	}
	return ret ? f : 0.;
}

// Tests whether two moving line segments intersect. Line segment A is moving arbitrary while line segment B is fixed at origin and points z+.
int jHitLines(const Vec3d &apos, const Quatd &arot, const Vec3d &avelo, const Vec3d &aomg, double alen, double blen, double dt, double *hitt){
	/* Line segment position

	              apose  apos1e
	                |----->|
	Line segment    |      |     Line segment at
	at start of --> |----->| <-- time + dt
	the frame       |      |
	                |----->|
	              apos   apos1
	
	The quadrilateral made by the four corners is generally not
	rectangular nor even planar. 
	*/

	Vec3d apose = apos + arot.trans(alen * vec3_001);
	Vec3d apos1 = apos + avelo * dt;
	Vec3d apos1e = apos1 + arot.quatrotquat(aomg * dt).trans(alen * vec3_001);

	double bc, cc;
	double t = jHitTriangle(apose - apos, apos1 - apos, Vec3d(0,0,0) - apos, Vec3d(0,0,blen) - apos, &bc, &cc);
	if(0 != t){
		if(hitt)
			*hitt = (1 - bc) * cc;
		return 1;
	}
	
	t = jHitTriangle(apos1 - apos1e, apose - apos1e, Vec3d(0,0,0) - apos1e, Vec3d(0,0,blen) - apos1e, &bc, &cc);
	if(0 != t){
		if(hitt)
			*hitt = (1 - bc) * (1. - cc);
		return 1;
	}
	return 0;
}

int jHitBoxes(const hitbox &hb1, const hitbox &hb2, const Vec3d &rvelo, const Vec3d &romg, double dt, double *hitt){
	static Vec3d lines[12][2];
	static bool init = false;
	if(!init){
		int i, j, k;
		for(i = 0; i < 3; i++) for(j = 0; j < 1; j++) for(k = 0; k < 1; k++){
			Vec3d (&v)[2] = lines[(i * 2 + j) * 2 + k];
			v[0][i] = j * 2 - 1;
			v[0][(i+1)%3] = k * 2 - 1;
			v[0][(i+2)%3] = -1.;
			v[1][i] = j * 2 - 1;
			v[1][(i+1)%3] = k * 2 - 1;
			v[1][(i+2)%3] = 1.;
		}
	}
	for(int i = 0; i < 12; i++) for(int j = 0; j < 12; j++){
//		Vec3d a = hb1.org + hb1.rot.trans(lines[i]);
//		jHitLines(a, hb1.rot, rvelo, romg, hb1.sc[i / 4] * 2., hb2.sc[j / 4] * 2., dt, hitt);
	}
	return 0;
}


#ifdef NDEBUG
#define OTDEBUG 0
#else
#define OTDEBUG 0
#endif

static int otjHitSphere_loops = 0, otjHitSphere_framecalls = 0, otjHitSphere_frameloops = 0;

static Entity *otjHitSphere_in(const otnt *root, const Vec3d &src, const Vec3d &dir, double dt, double rad, Vec3d *pos){
	int i;
	if(otjHitSphere_loops++, !jHitSphere(root->pos, root->rad + rad, src, dir, dt))
		return NULL;
	else for(i = 0; i < 2; i++){
		Entity *ret;
		if(root->leaf & (1<<i)){
			if(otjHitSphere_loops++, jHitSpherePos(root->a[i].t->pos, root->a[i].t->getHitRadius() + rad, src, dir, dt, NULL, pos))
				return root->a[i].t;
		}
		else if(ret = otjHitSphere_in(root->a[i].n, src, dir, dt, rad, pos))
			return ret;
	}
	return NULL;
}

Entity *otjHitSphere(const otnt *root, const Vec3d &src, const Vec3d &dir, double dt, double rad, Vec3d *pos){
	Entity *ret;
	if(!root)
		return NULL;
	otjHitSphere_loops = 0;
	ret = otjHitSphere_in(root, src, dir, dt, rad, pos);
	otjHitSphere_frameloops += otjHitSphere_loops;
	otjHitSphere_framecalls++;
#if 2 <= OTDEBUG
	printf("otjHS: %d\n", otjHitSphere_loops);
#endif
	return ret;
}



static Entity *otjEnumHitSphere_in(const otnt *root, const struct otjEnumHitSphereParam *param){
	const Vec3d *src = param->src;
	const Vec3d *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	Vec3d *pos = param->pos;
	int i;
	if(otjHitSphere_loops++, !jHitSphere(root->pos, root->rad + rad, *src, *dir, dt))
		return NULL;
	else for(i = 0; i < 2; i++){
		Entity *ret;
		if(root->leaf & (1<<i)){
			if(param->flags & OTJ_IGLIST){
				int j;
				for(j = 0; j < param->niglist; j++) if(root->a[i].t == param->iglist[j])
					goto gcontinue;
			}
/*			if(param->flags & OTJ_IGVFT){
				int j;
				for(j = 0; j < param->nigvft; j++) if(&root->a[i]->t.vft == param->igvft[j])
					goto gcontinue;
			}*/
			otjHitSphere_loops++;
			ret = root->a[i].t;
			
			// If an Entity gets deleted, the object tree may have a NULL pointer,
			// which case should be discarded from hit checking.
			if(!ret)
				continue;

			double radii = ret->getHitRadius();
			if(jHitSpherePos(ret->pos, radii + rad, *src, *dir, dt, NULL, pos)
				&& (!(param->flags & OTJ_CALLBACK) || param->callback(param, root->a[i].t)))
				return ret;
		}
		else if(ret = otjEnumHitSphere_in(root->a[i].n, param))
			return ret;
gcontinue:;
	}
	return NULL;
}

Entity *otjEnumHitSphere(const struct otjEnumHitSphereParam *param){
	Entity *ret;
	if(!param->root)
		return NULL;
	otjHitSphere_loops = 0;
	ret = otjEnumHitSphere_in(param->root, param);
	otjHitSphere_frameloops += otjHitSphere_loops;
	otjHitSphere_framecalls++;
#if 2 <= OTDEBUG
	printf("otjHS: %d\n", otjHitSphere_loops);
#endif
	return ret;
}

static Entity *otjEnumPointHitSphere_in(const otnt *root, const struct otjEnumHitSphereParam *param){
	const Vec3d *src = param->src;
	double radi = root->rad + param->rad;
	Vec3d *pos = param->pos;
	int i;
	if(otjHitSphere_loops++, radi * radi < VECSDIST(root->pos, *src))
		return NULL;
	else for(i = 0; i < 2; i++){
		Entity *ret;
		if(root->leaf & (1<<i)){
			// The WeakPtr can be NULL, in this case, just skip hit checking.
			if(root->a[i].t == NULL)
				continue;
			if(param->flags & OTJ_IGLIST){
				int j;
				for(j = 0; j < param->niglist; j++) if(root->a[i].t == param->iglist[j])
					goto gcontinue;
			}
/*			if(param->flags & OTJ_IGVFT){
				int j;
				for(j = 0; j < param->nigvft; j++) if(root->a[i]->t.vft == param->igvft[j])
					goto gcontinue;
			}*/
			radi = root->a[i].t->getHitRadius() + param->rad;
			otjHitSphere_loops++;
			if((root->a[i].t->pos - *src).slen() < radi * radi
				&& (!(param->flags & OTJ_CALLBACK) || param->callback(param, root->a[i].t)))
				return root->a[i].t;
		}
		else if(ret = otjEnumPointHitSphere_in(root->a[i].n, param))
			return ret;
gcontinue:;
	}
	return NULL;
}

Entity *otjEnumPointHitSphere(const struct otjEnumHitSphereParam *param){
	Entity *ret;
	if(!param->root)
		return NULL;
	otjHitSphere_loops = 0;
	ret = otjEnumPointHitSphere_in(param->root, param);
	otjHitSphere_frameloops += otjHitSphere_loops;
	otjHitSphere_framecalls++;
#if 2 <= OTDEBUG
	printf("otjHS: %d\n", otjHitSphere_loops);
#endif
	return ret;
}

static Entity *otjEnumNearestPoint_in(const otnt *root, const struct otjEnumHitSphereParam *param){
	const Vec3d *src = param->src;
	double radi[2];
	const Vec3d *pos[2];
	int i, s;
	for(i = 0; i < 2; i++)
		pos[i] = root->leaf & (1 << i) ? &root->a[i].t->pos : &root->a[i].n->pos;
	s = VECSDIST(*pos[0], *src) < VECSDIST(*pos[1], *src);
	for(i = 0; i < 2; i++){
		Entity *ret;
		int i1 = (i+s) % 2;
		if(root->leaf & (1<<i)){
			if(param->flags & OTJ_IGLIST){
				int j;
				for(j = 0; j < param->niglist; j++) if(root->a[i1].t == param->iglist[j])
					goto gcontinue;
			}
			if(!(param->flags & OTJ_CALLBACK) || param->callback(param, root->a[i1].t))
				return root->a[i1].t;
		}
		else if(ret = otjEnumNearestPoint_in(root->a[i1].n, param))
			return ret;
gcontinue:;
	}
	return NULL;
}

Entity *otjEnumNearestPoint(const struct otjEnumHitSphereParam *param){
	Entity *ret;
	if(!param->root)
		return NULL;
	otjHitSphere_loops = 0;
	ret = otjEnumNearestPoint_in(param->root, param);
	otjHitSphere_frameloops += otjHitSphere_loops;
	otjHitSphere_framecalls++;
#if 2 <= OTDEBUG
	printf("otjHS: %d\n", otjHitSphere_loops);
#endif
	return ret;
}














#define OT_PAIRED 0x80 // a flag indicating that the node already has a parent node.

static Entity *otEnumNearestPoint_in(const otnt *root, const Vec3d &src, const Entity *ignore){
	Entity *ret;
	double radi[2];
	const Vec3d *pos[2];
	int i, s;
	for(i = 0; i < 2; i++)
		pos[i] = root->leaf & (1 << i) ? &root->a[i].t->pos : &root->a[i].n->pos;
	s = (*pos[1] - src).slen() < (*pos[0] - src).slen();
	for(i = 0; i < 2; i++){
		int i1 = (i+s) % 2;
		if(root->leaf & (1<<i1)){
			if(root->a[i1].t == ignore || root->a[i1].t->otflag & 2)
				continue;
			return root->a[i1].t;
		}
		else if((ret = otEnumNearestPoint_in(root->a[i1].n, src, ignore))){
			return ret;
		}
	}
	return NULL;
}


/*
static otnt *ot_buildsub(warf_t *w, otnt *root){
	if(!root)
		return NULL;
	ot_buildsub(w, &root->a[0]->n);
	ot_buildsub(w, &root->a[1]->n);
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	return root;
}*/

#if 1 <= OTDEBUG
static int updatesub_invokes;
static statistician_t updatesub_stats;
#endif

static otnt *ot_update_int(otnt *root, WarField *w){
	Vec3d *pos[2];
	double rad[2];
	double len, newrad;
	int i;
	for(i = 0; i < 2; i++) if(root->leaf & (1 << i)){
		if(!root->a[i].t)
			return NULL;
		if(root->a[i].t->w != w)
			return NULL;
		rad[i] = root->a[i].t->getHitRadius();
		pos[i] = &root->a[i].t->pos;
	}
	else{
		rad[i] = root->a[i].n->rad;
		pos[i] = &root->a[i].n->pos;
	}
	Vec3d delta = *pos[0] - *pos[1];
	len = delta.len();

	/* If either one of the subnodes contains the other totally, just make
        current node same as larger subnode. */
	for(i = 0; i < 2; i++) if(len + rad[(i+1) % 2] < rad[i]){
		root->pos = *pos[i];
		root->rad = rad[i];
		return root;
	}

	// Avoid zero division
	if(len != 0.){
		Vec3d dn = delta / len;
		len += rad[0] + rad[1];
		newrad = len / 2.;
		len = newrad - rad[1];
		delta = dn * len;
		root->pos = delta + *pos[1];
		root->rad = newrad;
	}
	else{
		root->pos = *pos[0];
		root->rad = MAX(rad[0], rad[1]);
	}
	return root;
}

static otnt *ot_updatesub(WarField *w, otnt *root){
	if(!root)
		return NULL;
	if(root->a[0].n && !(root->leaf & 1) && !ot_updatesub(w, root->a[0].n))
		return NULL;
	if(root->a[1].n && !(root->leaf & 2) && !ot_updatesub(w, root->a[1].n))
		return NULL;
#if 1 <= OTDEBUG
	updatesub_invokes++;
#endif
#if 1
	if(!ot_update_int(root, w))
		return NULL;
#else
	{
		avec3_t *pos[2], delta, dn;
		double rad[2];
		double len, newrad;
		int i;
		for(i = 0; i < 2; i++) if(root->leaf & (1 << i)){
			if(!root->a[i]->t.active)
				return NULL;
			rad[i] = ((struct entity_private_static*)root->a[i]->t.vft)->getHitRadius;
			pos[i] = &root->a[i]->t.pos;
		}
		else{
			rad[i] = root->a[i]->n.rad;
			pos[i] = &root->a[i]->n.pos;
		}
		VECSUB(delta, *pos[0], *pos[1]);
		len = VECLEN(delta);

		/* If either one of the subnodes contains the other totally, just make
          current node same as larger subnode. */
		for(i = 0; i < 2; i++) if(len + rad[(i+1) % 2] < rad[i]){
			VECCPY(root->pos, *pos[i]);
			root->rad = rad[i];
			return root;
		}

		VECSCALE(dn, delta, 1. / len);
		len += rad[0] + rad[1];
		newrad = len / 2.;
		len = newrad - rad[1];
		VECSCALE(delta, dn, len);
		VECADD(root->pos, delta, *pos[1]);
		root->rad = newrad;
/*		root->a[0] = i < 0 ? pt : &ot[i];
		root->a[1] = jj < 0 ? pt3 : &ot[jj];
		root->leaf = (i < 0 ? 1 : 0) | (jj < 0 ? 2 : 0);*/
	}
#endif
	return root;
}

struct otiterator{
	int i, n;
	const int *o;
	WarField *w;
	WarField::EntityList::iterator it;
	otnt *ot;

	otiterator() : o(NULL){}
	otiterator(int an, WarField *aw, otnt *aot, const int *ao) : n(an), i(0), o(ao), w(aw), it(aw->el.begin()), ot(aot){
		// Skip invalid pointers
		while(it != w->el.end() && (!*it || (*it)->w != w))
			it++;
	}
	Vec3d getpos(){
		return it != w->el.end() ? *it ? (*it)->pos : Vec3d(0,0,0) : ot[i].pos;
	}
	double getrad(){
		return it != w->el.end() ? *it ? (*it)->getHitRadius() : 0. : ot[i].rad;
	}
	otiterator next(){
		otiterator ret = *this;
		return ++ret;
	}
	otiterator &operator++(){
		if(it != w->el.end()) do{
			it++;
		}while(it != w->el.end() && (!*it || (*it)->w != w));
		else
			i++;
		return *this;
	}
	operator bool(){
		return it != w->el.end() && *it || i != *o;
	}
	bool paired()const{
		return it != w->el.end() ? *it && (*it)->otflag & 2 : ot[i].leaf & OT_PAIRED;
	}
	bool get(otnt::unode &u){
		if(it != w->el.end()){
			u.t = *it;
		}
		else
			u.n = &ot[i];
		return it != w->el.end();
	}
	void pair(){
		if(it != w->el.end())
			(*it)->otflag |= 2;
		else
			ot[i].leaf |= OT_PAIRED;
	}
};

otnt *ot_build(WarSpace *w, double dt){
	class Thrower{
	public:
		static void thrower(int h){
			throw h;
		}
	};
	void (*prevHandler)(int) = signal(SIGSEGV, Thrower::thrower);
	try{
	int i, n = 0, m = 0, o, loops = 0;
	otnt *ot = w->ot;
#if 1 <= OTDEBUG
	timemeas_t tm;
#endif
#if 1 <= OTDEBUG
	TimeMeasStart(&tm);
#endif
	if(w->otroot && fmod(w->war_time(), 2.) < fmod(w->war_time() + dt, 2.)){
#if 1 <= OTDEBUG
		updatesub_invokes = 0;
#endif
		if(ot_updatesub(w, w->otroot)){
#if 1 <= OTDEBUG
			double t;
			t = TimeMeasLap(&tm);
			PutStats(&updatesub_stats, t);
			printf("ot_update loops %d, %lg s, avg %lg s\n", updatesub_invokes, t, updatesub_stats.avg);
#endif
			return w->ot;
		}
	}
	otjHitSphere_framecalls = otjHitSphere_frameloops = 0;
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it && (*it)->w == w){
		(*it)->otflag &= ~2;
		n++;
	}

	/* The case n == 1 will fail building up tree. */
	if(n <= 1){
		w->oti = 0;
		w->otroot = NULL;
		return NULL;
	}

/*	for(i = n; i; i--)
		m += i;*/
	m = n * n;
	if(w->ots < m){
		ot = (otnt*)malloc(m * sizeof *ot);
		for(int i = 0; i < w->ots; i++){
			new(&ot[i]) otnt(w->ot[i]);
			w->ot[i].~otnt();
		}
		for(int i = w->ots; i < m; i++)
			new(&ot[i]) otnt();
		free(w->ot);
		w->ot = ot;
		w->ots = m;
	}
#if 1
#define PAIRED(i,pt) ((i) < 0 ? (pt)->otflag & 2 : (ot[i]).leaf & OT_PAIRED)
#define PAIR(i,pt) ((i) < 0 ? ((pt)->otflag |= 2) : ((ot[i]).leaf |= OT_PAIRED))
	o = 0;
	otiterator it(n, w, ot, &o);
//	for(pt = w->el, i = -n; i < o; (i < 0 ? pt = pt->next : 0), (i < 0 && pt->w != w ? 0 : i++)) if(i < 0 && pt->w != w){
	for(; it; ++it) if(!it.paired()){
		double slen, best = 1e15, rad;
		const Vec3d pos = it.getpos();
		Entity *pt2, *pt3;
		rad = it.getrad();
		pt3 = NULL;
		bool found = false;
		otiterator it3;
		for(otiterator it2 = it.next(); it2; ++it2) if(!it2.paired()){
			double rad_2 = it2.getrad();
			const Vec3d pos2 = it2.getpos();
			slen = (pos - pos2).slen() + rad * rad + rad_2 * rad_2;
			if(slen < best){
				it3 = it2;
/*				jj = it2.i;
				if(j < 0)
					pt3 = pt2;*/
				best = slen;
				found = true;
			}
			loops++;
		}
		if(found){
			avec3_t delta, dn;
			double len, newrad;
			double rad_2;
			const avec3_t *pos2;
/*			if(jj < 0){
				rad_2 = ((struct entity_private_static*)pt3->vft)->getHitRadius;
				pos2 = &pt3->pos;
			}
			else{
				rad_2 = ot[jj].rad;
				pos2 = &ot[jj].pos;
			}
			VECSUB(delta, *pos, *pos2);
			len = VECLEN(delta);
			VECSCALE(dn, delta, 1. / len);
			len += rad + rad_2;
			newrad = len / 2.;
			len = newrad - rad_2;
			VECSCALE(delta, dn, len);
			VECADD(ot[o].pos, delta, *pos2);
			ot[o].rad = newrad;*/
			assert(o < n);
			ot[o].leaf = !!it.get(ot[o].a[0]) | (!!it3.get(ot[o].a[1]) << 1);
			ot_update_int(&ot[o], w);
#if 2 <= OTDEBUG
			printf("ot pair[%d] (%d,%d)\n", o, it.i, it3.i);
#endif
			o++;
			it.pair();
			it3.pair();
		}
	}
	w->ot = ot;
	w->oti = o;
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it){
		(*it)->otflag &= ~2;
	}
#if 1 <= OTDEBUG
	printf("ot nobjs %d, nodes %d, loops %d, %lg s\n", n, o, loops, TimeMeasLap(&tm));
#endif
#else
	i = 0;
	for(pt = w->tl; pt; pt = pt->next) if(!(pt->active & 2)){
		struct entity_private_static *vft = (struct entity_private_static*)pt->vft;
		entity_t *pt2, *pt3 = NULL;
		double slen, best = HUGE_VAL;
		for(pt2 = pt->next; pt2; pt2 = pt2->next) if(!(pt2->active & 2)){
			struct entity_private_static *vft2 = (struct entity_private_static*)pt2->vft;
			slen = VECSDIST(pt->pos, pt2->pos) + vft->getHitRadius * vft->getHitRadius + vft2->getHitRadius * vft2->getHitRadius;
			if(slen < best){
				pt3 = pt2;
				best = slen;
			}
			loops++;
		}
		if(pt3){
			struct entity_private_static *vft2 = (struct entity_private_static*)pt3->vft;
			avec3_t delta, dn;
			double len, rad;
			VECSUB(delta, pt->pos, pt3->pos);
			len = VECLEN(delta);
			VECSCALE(dn, delta, 1. / len);
			if(len + vft2->getHitRadius < vft->getHitRadius){
				VECCPY(ot[i].pos, pt->pos);
				ot[i].rad = vft->getHitRadius;
			}
			else if(len + vft->getHitRadius < vft2->getHitRadius){
				VECCPY(ot[i].pos, pt3->pos);
				ot[i].rad = vft2->getHitRadius;
			}
			else{
				len += vft->getHitRadius + vft2->getHitRadius;
				rad = len / 2.;
				len = rad - vft2->getHitRadius;
				VECSCALE(delta, dn, len);
				VECADD(ot[i].pos, delta, pt3->pos);
				ot[i].rad = rad;
			}
			ot[i].a[0] = pt;
			ot[i].a[1] = pt3;
			ot[i].leaf = 3;
			i++;
			pt->active |= 2;
			pt3->active |= 2;
		}
	}
	l = i;
#define PAIRED(otn) ((otn).leaf & 128)
#define PAIR(otn) ((otn).leaf |= 128)
	o = l;
	do{
		l = o;
		k = 0;
		for(i = 0; i < l; i++) if(!PAIRED(ot[i])){
			double slen, best = HUGE_VAL;
			otnt *pn = &ot[i], *pn3 = NULL;
			int j;
			for(j = i+1; j < o; j++) if(!PAIRED(ot[j])){
				otnt *pn2 = &ot[j];
				slen = VECSDIST(pn->pos, pn2->pos) + pn->rad * pn->rad + pn2->rad * pn2->rad;
				if(slen < best){
					pn3 = pn2;
					best = slen;
				}
				loops++;
			}
			if(pn3){
				avec3_t delta, dn;
				double len, rad;
				VECSUB(delta, pn->pos, pn3->pos);
				len = VECLEN(delta);
				VECSCALE(dn, delta, 1. / len);
				len += pn->rad + pn3->rad;
				rad = len / 2.;
				len = rad - pn3->rad;
				VECSCALE(delta, dn, len);
				VECADD(ot[o].pos, delta, pn3->pos);
				ot[o].rad = rad;
				ot[o].a[0] = pn;
				ot[o].a[1] = pn3;
				ot[o].leaf = 0;
#if 2 <= OTDEBUG
				printf("ot pair[%d]%d (%d,%d)\n", o, k, pn - ot, pn3 - ot);
#endif
				o++;
				k++;
				PAIR(*pn);
				PAIR(*pn3);
			}
		}
	} while(k);
	w->oti = o;
	for(pt = w->tl; pt; pt = pt->next){
		pt->active &= ~2;
	}
#if 1 <= OTDEBUG
	printf("ot nobjs %d, nodes %d, loops %d\n", n, o, loops);
#endif
#endif
	/* The last node is not necessarily the root node, but it is. */
	w->otroot = &w->ot[o-1];
	signal(SIGSEGV, prevHandler);
	return w->otroot;
	}
	catch(...){
		FILE *fp = fopen("otnt.dump", "wb");
		if(fp){
			fwrite(w->ot, sizeof *w->ot, w->ots, fp);
			fclose(fp);
		}
	}
	signal(SIGSEGV, prevHandler);
}

otnt *ot_check(WarSpace *w, double dt){
	for(WarField::EntityList::iterator it = w->el.begin(); it != w->el.end(); it++) if(*it && (*it)->w != w)
		return ot_build(w, dt);
	return w->ot;
}

#ifdef _WIN32
static void circle(Vec3d &org, double s, Mat4d &rot){
	int i;
		double (*cuts)[2];
		double c;
		cuts = CircleCuts(32);
		c = sqrt(1. - s * s);
		glPushMatrix();
		glLoadMatrixd(rot);
		gldMultQuat(Quatd::direction(org));
		glBegin(GL_LINE_LOOP);
		for(i = 0; i < 32; i++){
			glVertex3d(s * cuts[i][0], s * cuts[i][1], c);
		}
		glEnd();
		glPopMatrix();
}
#endif

#if 0 || 1 <= OTDEBUG
static int ot_nodes[10];
static double ot_volume[10];
static int ot_statsub(otnt *root, int level){
	if(!root || 10 <= level)
		return 0;
	if(root->a[0].t && !(root->leaf & 1))
		ot_statsub(root->a[0].n, level + 1);
	if(root->a[1].t && !(root->leaf & 2))
		ot_statsub(root->a[1].n, level + 1);
	ot_nodes[level]++;
	ot_volume[level] += root->rad * root->rad * root->rad;
}
#endif

void ot_draw(WarSpace *w, wardraw_t *wd){
	int i, n;
	Viewer *vw = wd->vw;
#if 0 || 1 && !OTDEBUG
	return;
#else
	if(!w->otroot)
		return;
	for(n = 0; n < w->oti; n++){
		otnt *pn = &w->ot[n];
		Mat4d mat;
		double ratio, tangent, c, s;
		Vec3d org = pn->pos - vw->pos;
		ratio = pn->rad / org.len();

		glColor4ub(255,0,255,255);
		if(pn->a[0].t && pn->a[1].t){
			Vec3d pos;
			glBegin(GL_LINES);
			glVertex3dv(pn->leaf & 1 ? pn->a[0].t->pos : pn->a[0].n->pos);
			glVertex3dv(pn->leaf & 2 ? pn->a[1].t->pos : pn->a[1].n->pos);
			glEnd();
			if(pn->leaf & 1){
				pos = pn->a[0].t->pos - vw->pos;
				circle(pos, (pn->leaf & 1 ? pn->a[0].t->getHitRadius() : pn->a[1].n->rad) / pos.len(), vw->rot);
			}
			if(pn->leaf & 2){
				pos = (pn->leaf & 2 ? pn->a[1].t->pos : pn->a[1].n->pos) - vw->pos;
				circle(pos, (pn->leaf & 2 ? pn->a[1].t->getHitRadius() : pn->a[1].n->rad) / pos.len(), vw->rot);
			}
		}

		if(1 < ratio)
			continue;
		tangent = asin(ratio);
		c = cos(tangent);
		s = ratio;
/*		glPushMatrix();
		glLoadMatrixd(vw->rot);
		glTranslated(-vw->pos[0], -vw->pos[1], -vw->pos[2]);
		gldTranslate3dv(org);
		glMultMatrixd(vw->irot);
		gldScaled(cs->rad);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		glPopMatrix();*/

		circle(org, s, vw->rot);
	}
	glPushMatrix();
	glLoadIdentity();
	glTranslated(-.5, -.5, -1.);
	glScaled(.1, 1., 1.);
	for(i = 0; i < 10; i++){
		ot_volume[i] = 0.;
		ot_nodes[i] = 0;
	}
	ot_statsub(w->otroot, 0);
	{
		double m = 1e-10;
		int mi = 0;
		for(i = 0; i < 10; i++){
			if(m < ot_volume[i])
				m = ot_volume[i];
			if(mi < ot_nodes[i])
				mi = ot_nodes[i];
		}
		glColor4ub(255,255,0,255);
		glBegin(GL_LINE_STRIP);
		for(i = 0; i < 10; i++)
			glVertex2d(i, ot_volume[i] ? 1. + .1 * log10(ot_volume[i] / m) : 0.);
		glEnd();
		glColor4ub(0,255,255,255);
		glBegin(GL_LINE_STRIP);
		for(i = 0; i < 10; i++)
			glVertex2d(i, (double)ot_nodes[i] / mi);
		glEnd();
	}
	glPopMatrix();
#endif
}
