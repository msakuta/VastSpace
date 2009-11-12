#include "judge.h"
#include "astro.h"
#include "viewer.h"
extern "C"{
#include <clib/timemeas.h>
#include <clib/avec3.h>
#include <clib/amat4.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
#include <clib/stats.h>
}
#include <stddef.h>

int jHitSphere(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt){
	return jHitSpherePos(obj, radius, src, dir, dt, NULL, NULL);
}

/* determine intersection of a sphere shell and a ray. it's equivalent to inter-sphere hit detection,
  when the radius argument is sum of the two spheres. */
int jHitSpherePos(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt, double *retf, Vec3d *pos){
	double b, c, D, d, t0, t1, dirslen;
	int ret;

	Vec3d del = src - obj;

	/* scalar product of the ray and the vector. */
	b = dir.sp(del);

	/* ??? */
	dirslen = dir.slen();
	c = dirslen * (del.slen() - radius * radius);

	/* discriminant?? */
	D = b * b - c;
	if(D <= 0)
		return 0;

	d = sqrt(D);

	/* we need vector equation's parameter value to determine hitness with line segment */
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


int jHitBox(const double org[3], const double scale[3], const double rot[4], const double src[3], const double dir[3], double mint, double maxt, double *ret, double (*retp)[3], avec3_t *retn){
	int i, reti = 0;
	amat4_t imat, mat;
	avec3_t lsrc, ldir;

	quat2imat(&imat, rot);
/*	for(i = 0; i < 3; i++)
		VECSCALEIN(&imat[i*4], 1. / scale[i]);*/
/*	MAT4SCALE(imat, 1. / scale[0], 1. / scale[1], 1. / scale[2]);*/

	quat2mat(&mat, rot);
/*	MAT4SCALE(mat, scale[0], scale[1], scale[2]);*/
/*	{
		amat3_t mat3, imat3;
		MAT4TO3(mat3, mat);
		matinv(imat3, mat3);
		MAT3TO4(imat, imat3);
	}*/
	VECCPY(&mat[12], org);

/*	MAT4TRANSLATE(imat, -org[0], -org[1], -org[2]);*/
/*	VECSCALE(&imat[12], org, -1);*/

	{
		avec3_t dr;
		VECSUB(dr, src, org);
		mat4dvp3(lsrc, imat, dr);
		mat4dvp3(ldir, imat, dir);
	}

	{
		double f, best = maxt;
		avec3_t hit;
		for(i = 0; i < 3; i++){
			int s = ldir[i] < 0. ? -1 : 1, a = i;
			f = (-lsrc[a] - s * scale[a]) / ldir[a];
			if(!(mint <= f && f < best))
				continue;
			VECSCALE(hit, ldir, f);
			VECADDIN(hit, lsrc);
			if(-scale[(a+1)%3] < hit[(a+1)%3] && hit[(a+1)%3] < scale[(a+1)%3] && -scale[(a+2)%3] < hit[(a+2)%3] && hit[(a+2)%3] < scale[(a+2)%3]){
				if(ret) *ret = f;
				if(retp){
/*					mat4vp3(*retp, mat, hit);*/
					VECSCALE(*retp, dir, f);
					VECADDIN(*retp, src);
				}
				if(retn){
/*					quatrot(*retn, rot, a == 0 ? avec3_100 : a == 1 ? avec3_010 : avec3_001);*/
					VECNORM(*retn, &mat[a * 4]);
					VECSCALEIN(*retn, -s);
				}
				best = f;
				reti = 1;
			}
		}
	}
	return reti;
}

#if 0
#define OTDEBUG 0

static int otjHitSphere_loops = 0, otjHitSphere_framecalls = 0, otjHitSphere_frameloops = 0;

static entity_t *otjHitSphere_in(const otnt *root, const avec3_t *src, const avec3_t *dir, double dt, double rad, avec3_t *pos){
	int i;
	if(otjHitSphere_loops++, !jHitSphere(root->pos, root->rad + rad, *src, *dir, dt))
		return NULL;
	else for(i = 0; i < 2; i++){
		entity_t *ret;
		if(root->leaf & (1<<i)){
			if(otjHitSphere_loops++, jHitSpherePos(root->a[i]->t.pos, ((struct entity_private_static *)root->a[i]->t.vft)->hitradius + rad, *src, *dir, dt, NULL, *pos))
				return &root->a[i]->t;
		}
		else if(ret = otjHitSphere_in(&root->a[i]->n, src, dir, dt, rad, pos))
			return ret;
	}
	return NULL;
}

entity_t *otjHitSphere(const otnt *root, const avec3_t *src, const avec3_t *dir, double dt, double rad, avec3_t *pos){
	entity_t *ret;
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



static entity_t *otjEnumHitSphere_in(const otnt *root, const struct otjEnumHitSphereParam *param){
	const avec3_t *src = param->src;
	const avec3_t *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	avec3_t *pos = param->pos;
	int i;
	if(otjHitSphere_loops++, !jHitSphere(root->pos, root->rad + rad, *src, *dir, dt))
		return NULL;
	else for(i = 0; i < 2; i++){
		entity_t *ret;
		if(root->leaf & (1<<i)){
			if(param->flags & OTJ_IGLIST){
				int j;
				for(j = 0; j < param->niglist; j++) if(&root->a[i]->t == param->iglist[j])
					goto gcontinue;
			}
			if(param->flags & OTJ_IGVFT){
				int j;
				for(j = 0; j < param->nigvft; j++) if(&root->a[i]->t.vft == param->igvft[j])
					goto gcontinue;
			}
			otjHitSphere_loops++;
			if(jHitSpherePos(root->a[i]->t.pos, ((struct entity_private_static *)root->a[i]->t.vft)->hitradius + rad, *src, *dir, dt, NULL, *pos)
				&& (!(param->flags & OTJ_CALLBACK) || param->callback(param, &root->a[i]->t)))
				return &root->a[i]->t;
		}
		else if(ret = otjEnumHitSphere_in(&root->a[i]->n, param))
			return ret;
gcontinue:;
	}
	return NULL;
}

entity_t *otjEnumHitSphere(const struct otjEnumHitSphereParam *param){
	entity_t *ret;
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

static entity_t *otjEnumPointHitSphere_in(const otnt *root, const struct otjEnumHitSphereParam *param){
	const avec3_t *src = param->src;
	double radi = root->rad + param->rad;
	avec3_t *pos = param->pos;
	int i;
	if(otjHitSphere_loops++, radi * radi < VECSDIST(root->pos, *src))
		return NULL;
	else for(i = 0; i < 2; i++){
		entity_t *ret;
		if(root->leaf & (1<<i)){
			if(param->flags & OTJ_IGLIST){
				int j;
				for(j = 0; j < param->niglist; j++) if(&root->a[i]->t == param->iglist[j])
					goto gcontinue;
			}
			if(param->flags & OTJ_IGVFT){
				int j;
				for(j = 0; j < param->nigvft; j++) if(root->a[i]->t.vft == param->igvft[j])
					goto gcontinue;
			}
			radi = ((struct entity_private_static *)root->a[i]->t.vft)->hitradius + param->rad;
			otjHitSphere_loops++;
			if(VECSDIST(root->a[i]->t.pos, *src) < radi * radi
				&& (!(param->flags & OTJ_CALLBACK) || param->callback(param, &root->a[i]->t)))
				return &root->a[i]->t;
		}
		else if(ret = otjEnumPointHitSphere_in(&root->a[i]->n, param))
			return ret;
gcontinue:;
	}
	return NULL;
}

entity_t *otjEnumPointHitSphere(const struct otjEnumHitSphereParam *param){
	entity_t *ret;
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

static entity_t *otjEnumNearestPoint_in(const otnt *root, const struct otjEnumHitSphereParam *param){
	const avec3_t *src = param->src;
	double radi[2];
	const avec3_t *pos[2];
	int i, s;
	for(i = 0; i < 2; i++)
		pos[i] = root->leaf & (1 << i) ? &root->a[i]->t.pos : &root->a[i]->n.pos;
	s = VECSDIST(*pos[0], *src) < VECSDIST(*pos[1], *src);
	for(i = 0; i < 2; i++){
		entity_t *ret;
		int i1 = (i+s) % 2;
		if(root->leaf & (1<<i)){
			if(param->flags & OTJ_IGLIST){
				int j;
				for(j = 0; j < param->niglist; j++) if(&root->a[i1]->t == param->iglist[j])
					goto gcontinue;
			}
			if(!(param->flags & OTJ_CALLBACK) || param->callback(param, &root->a[i1]->t))
				return &root->a[i1]->t;
		}
		else if(ret = otjEnumNearestPoint_in(&root->a[i1]->n, param))
			return ret;
gcontinue:;
	}
	return NULL;
}

static entity_t *otjEnumNearestPoint(const struct otjEnumHitSphereParam *param){
	entity_t *ret;
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
















static entity_t *otEnumNearestPoint_in(const otnt *root, const avec3_t *src, const entity_t *ignore){
	entity_t *ret;
	double radi[2];
	const avec3_t *pos[2];
	int i, s;
	for(i = 0; i < 2; i++)
		pos[i] = root->leaf & (1 << i) ? &root->a[i]->t.pos : &root->a[i]->n.pos;
	s = VECSDIST(*pos[1], *src) < VECSDIST(*pos[0], *src);
	for(i = 0; i < 2; i++){
		int i1 = (i+s) % 2;
		if(root->leaf & (1<<i1)){
			if(&root->a[i1]->t == ignore || root->a[i1]->t.active & 2)
				continue;
			return &root->a[i1]->t;
		}
		else if((ret = otEnumNearestPoint_in(&root->a[i1]->n, src, ignore))){
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

static void ot_update_int(otnt *root){
	avec3_t *pos[2], delta, dn;
	double rad[2];
	double len, newrad;
	int i;
	for(i = 0; i < 2; i++) if(root->leaf & (1 << i)){
		if(!root->a[i]->t.active)
			return NULL;
		rad[i] = ((struct entity_private_static*)root->a[i]->t.vft)->hitradius;
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
}

static otnt *ot_updatesub(warf_t *w, otnt *root){
	if(!root)
		return NULL;
	if(root->a[0] && !(root->leaf & 1) && !ot_updatesub(w, &root->a[0]->n))
		return NULL;
	if(root->a[1] && !(root->leaf & 2) && !ot_updatesub(w, &root->a[1]->n))
		return NULL;
#if 1 <= OTDEBUG
	updatesub_invokes++;
#endif
#if 1
	ot_update_int(root);
#else
	{
		avec3_t *pos[2], delta, dn;
		double rad[2];
		double len, newrad;
		int i;
		for(i = 0; i < 2; i++) if(root->leaf & (1 << i)){
			if(!root->a[i]->t.active)
				return NULL;
			rad[i] = ((struct entity_private_static*)root->a[i]->t.vft)->hitradius;
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

otnt *ot_build(warf_t *w, double dt){
	entity_t *pt;
	int i, n = 0, m = 0, o, loops = 0;
	otnt *ot = w->ot;
#if 1 <= OTDEBUG
	timemeas_t tm;
#endif
#if 1 <= OTDEBUG
	TimeMeasStart(&tm);
#endif
	if(w->otroot && fmod(w->war_time, 2.) < fmod(w->war_time + dt, 2.)){
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
	for(pt = w->tl; pt; pt = pt->next){
		pt->active &= ~2;
		n++;
	}

	/* The case n == 1 will fail building up tree. */
	if(n <= 1)
		return;

	for(i = n; i; i--)
		m += i;
	if(0 && w->otroot){
		ot = malloc(m * sizeof *ot);
	}
	else if(w->ots < m)
		ot = realloc(ot, (w->ots = m) * sizeof *ot);
#if 1
#define PAIRED(i,pt) ((i) < 0 ? (pt)->active & 2 : (ot[i]).leaf & 128)
#define PAIR(i,pt) ((i) < 0 ? ((pt)->active |= 2) : ((ot[i]).leaf |= 128))
	o = 0;
	for(pt = w->tl, i = -n; i < o; (i < 0 ? pt = pt->next : 0), i++) if(!PAIRED(i, pt)){
		double slen, best = 1e15, rad;
		const avec3_t *pos;
		entity_t *pt2, *pt3;
		int j, jj = i;
		if(i < 0){
			rad = ((struct entity_private_static*)pt->vft)->hitradius;
			pos = &pt->pos;
		}
		else{
			rad = ot[i].rad;
			pos = &ot[i].pos;
		}
		if(0 && w->otroot && i < -1){
			pt3 = otEnumNearestPoint_in(w->otroot, pt->pos, pt);
			jj = -1;
		}
		else
			pt3 = NULL;
		if(!pt3) for(pt2 = i < 0 ? pt->next : NULL, j = i+1; j < o; (j < 0 ? pt2 = pt2->next : 0), j++) if(!PAIRED(j, pt2)){
			double rad_2;
			const avec3_t *pos2;
			otnt *pn2 = &ot[j];
			if(j < 0){
				rad_2 = ((struct entity_private_static*)pt2->vft)->hitradius;
				pos2 = &pt2->pos;
			}
			else{
				rad_2 = ot[j].rad;
				pos2 = &ot[j].pos;
			}
			slen = VECSDIST(*pos, *pos2) + rad * rad + rad_2 * rad_2;
			if(slen < best){
				jj = j;
				if(j < 0)
					pt3 = pt2;
				best = slen;
			}
			loops++;
		}
		if(jj != i){
			avec3_t delta, dn;
			double len, newrad;
			double rad_2;
			const avec3_t *pos2;
/*			if(jj < 0){
				rad_2 = ((struct entity_private_static*)pt3->vft)->hitradius;
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
			ot[o].a[0] = i < 0 ? pt : &ot[i];
			ot[o].a[1] = jj < 0 ? pt3 : &ot[jj];
			ot[o].leaf = (i < 0 ? 1 : 0) | (jj < 0 ? 2 : 0);
			ot_update_int(&ot[o]);
#if 2 <= OTDEBUG
			printf("ot pair[%d] (%d,%d)\n", o, i, jj);
#endif
			o++;
			PAIR(i, pt);
			PAIR(jj, pt3);
		}
	}
	w->ot = ot;
	w->oti = o;
	for(pt = w->tl; pt; pt = pt->next){
		pt->active &= ~2;
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
			slen = VECSDIST(pt->pos, pt2->pos) + vft->hitradius * vft->hitradius + vft2->hitradius * vft2->hitradius;
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
			if(len + vft2->hitradius < vft->hitradius){
				VECCPY(ot[i].pos, pt->pos);
				ot[i].rad = vft->hitradius;
			}
			else if(len + vft->hitradius < vft2->hitradius){
				VECCPY(ot[i].pos, pt3->pos);
				ot[i].rad = vft2->hitradius;
			}
			else{
				len += vft->hitradius + vft2->hitradius;
				rad = len / 2.;
				len = rad - vft2->hitradius;
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
	return w->otroot;
}

static void circle(avec3_t org, double s, amat4_t rot){
	int i;
		double (*cuts)[2];
		double c;
		cuts = CircleCuts(32);
		c = sqrt(1. - s * s);
		glPushMatrix();
		glLoadMatrixd(rot);
		directrot(org, avec3_000, NULL);
		glBegin(GL_LINE_LOOP);
		for(i = 0; i < 32; i++){
			glVertex3d(s * cuts[i][0], s * cuts[i][1], c);
		}
		glEnd();
		glPopMatrix();
}

#if 0 || 1 <= OTDEBUG
static int ot_nodes[10];
static double ot_volume[10];
static int ot_statsub(otnt *root, int level){
	if(!root || 10 <= level)
		return 0;
	if(root->a[0] && !(root->leaf & 1))
		ot_statsub(&root->a[0]->n, level + 1);
	if(root->a[1] && !(root->leaf & 2))
		ot_statsub(&root->a[1]->n, level + 1);
	ot_nodes[level]++;
	ot_volume[level] += root->rad * root->rad * root->rad;
}
#endif

void ot_draw(warf_t *w, wardraw_t *wd){
	int i, n;
	Viewer *vw = wd->vw;
#if 0 || 1 && !OTDEBUG
	return;
#else
	if(!w->otroot)
		return;
	for(n = 0; n < w->oti; n++){
		otnt *pn = &w->ot[n];
		amat4_t mat;
		double ratio, tangent, c, s;
		avec3_t org;
		VECSUB(org, pn->pos, vw->pos);
		ratio = pn->rad / VECLEN(org);

		glColor4ub(255,0,255,255);
		if(pn->a[0] && pn->a[1]){
			avec3_t pos;
			glBegin(GL_LINES);
			glVertex3dv(pn->leaf & 1 ? pn->a[0]->t.pos : pn->a[0]->n.pos);
			glVertex3dv(pn->leaf & 2 ? pn->a[1]->t.pos : pn->a[1]->n.pos);
			glEnd();
			if(pn->leaf & 1){
				VECSUB(pos, pn->a[0]->t.pos, vw->pos);
				circle(pos, (pn->leaf & 1 ? ((struct entity_private_static*)pn->a[0]->t.vft)->hitradius : pn->a[1]->n.rad) / VECLEN(pos), vw->rot);
			}
			if(pn->leaf & 2){
				VECSUB(pos, pn->leaf & 2 ? pn->a[1]->t.pos : pn->a[1]->n.pos, vw->pos);
				circle(pos, (pn->leaf & 2 ? ((struct entity_private_static*)pn->a[1]->t.vft)->hitradius : pn->a[1]->n.rad) / VECLEN(pos), vw->rot);
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
#endif
