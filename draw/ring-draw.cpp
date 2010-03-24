#include "../astrodraw.h"
#include "../judge.h"
extern "C"{
#include <clib/amat3.h>
#include <clib/rseq.h>
#include <clib/timemeas.h>
//#include <clib/gl/cull.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/c.h>
}
#include <cpplib/gl/cullplus.h>
#include <gl/glext.h>



#define ASTEROIDLIST 1

static void drawasteroid(const double org[3], const Quatd &qrot, double rad, unsigned long seed){
	int i, j;
#if ASTEROIDLIST
	static GLuint varies[8] = {0};
#else
	static struct asteroidcache{
		double pos[8][9][3], norm[8][9][3];
		double tpos[3], tnorm[3];
		double bpos[3], bnorm[3];
	} caches[8];
	static char varies[numof(caches)];
#endif
	if(!varies[seed %= numof(varies)]){
	double *pos[8][9]; /* indirect vector referencing to simplify normal generation */
#if ASTEROIDLIST
	double posv[8][7][3], norm[8][8][3];
	double tpos[3], tnorm[3];
	double bpos[3], bnorm[3];
	glNewList(varies[seed] = glGenLists(1), GL_COMPILE);
	{
		GLfloat dif[] = {.5, .475, .45, 1.}, amb[] = {.1, .1, .05, 1.};
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	}
#else
	double (*posv)[8][3] = caches[seed].pos, (*norm)[8][3] = caches[seed].norm;
	double *tpos = caches[seed].tpos, *tnorm = caches[seed].tnorm;
	double *bpos = caches[seed].bpos, *bnorm = caches[seed].bnorm;
#endif
	for(i = 0; i < 8; i++){
		pos[i][0] = tpos;
		for(j = 1; j < 8; j++)
			pos[i][j] = posv[i][j-1];
		pos[i][8] = bpos;
	}
	for(i = 0; i < 8; i++) for(j = 1; j < 8; j++){
		double ci, cj, si, sj, ci1, cj1, si1, sj1;
		struct random_sequence rs;
		ci = cos(i * 2 * M_PI / 8);
		si = sin(i * 2 * M_PI / 8);
		cj = cos(j * M_PI / 8);
		sj = sin(j * M_PI / 8);
		ci1 = cos((i+1) * 2 * M_PI / 8);
		si1 = sin((i+1) * 2 * M_PI / 8);
		cj1 = cos((j+1) * M_PI / 8);
		sj1 = sin((j+1) * M_PI / 8);
		init_rseq(&rs, seed + i + j * 8);
		pos[i][j][0] = (drseq(&rs) + 1.) * ci * sj;
		pos[i][j][1] = (drseq(&rs) + 1.) * cj;
		pos[i][j][2] = (drseq(&rs) + 1.) * si * sj;
	}
	{
		struct random_sequence rs;
		init_rseq(&rs, seed + 1221);
		tpos[0] = 0.;
		tpos[1] = (drseq(&rs) + 1.);
		tpos[2] = 0.;
		bpos[0] = 0.;
		bpos[1] = -(drseq(&rs) + 1.);
		bpos[2] = 0.;
	}
	for(i = 0; i < 8; i++) for(j = 1; j < 8; j++){
		int i1 = (i+1)%8, j0 = j-1, j1 = (j+1)%9, in = (i-1+8)%8, jn = (j-1+9)%9;
		avec3_t dx, dy, vec;
		VECSUB(dx, pos[i1][j], pos[i][j]);
		VECSUB(dy, pos[i][j1], pos[i][j]);
		VECVP(vec, dx, dy);
		VECNORMIN(vec);
		VECCPY(norm[i][j0], vec);
		VECSUB(dy, pos[i][jn], pos[i][j]);
		VECVP(vec, dy, dx);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECSUB(dx, pos[in][j], pos[i][j]);
		VECVP(vec, dx, dy);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECSUB(dy, pos[i][j1], pos[i][j]);
		VECVP(vec, dy, dx);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECNORMIN(norm[i][j0]);
/*		glBegin(GL_LINES);
		glVertex3dv(pos[i][j]);
		{
			avec3_t sum;
			VECADD(sum, pos[i][j], norm[i][j]);
			glVertex3dv(sum);
		}
		glEnd();
		*/
	}
	for(i = 0; i < 8; i++) for(j = 1; j < 7; j++){
		int i1 = (i+1)%8, j1 = (j+1)%9, j0 = j-1, j01 = j;
		glBegin(GL_TRIANGLE_STRIP);
		glNormal3dv(norm[i][j0]);
		glVertex3dv(pos[i][j]);
		glNormal3dv(norm[i1][j0]);
		glVertex3dv(pos[i1][j]);
		glNormal3dv(norm[i][j01]);
		glVertex3dv(pos[i][j1]);
		glNormal3dv(norm[i1][j01]);
		glVertex3dv(pos[i1][j1]);
		glEnd();
	}
	{
		VECNULL(bnorm);

		/* bottom cap */
		for(i = 0; i < 8; i++){
			avec3_t dx, dy, vec;
			int i1 = (i+1)%8;
			VECSUB(dx, pos[i][7], bpos);
			VECSUB(dy, pos[i1][7], bpos);
			VECVP(vec, dx, dy);
			VECNORMIN(vec);
			VECADDIN(bnorm, vec);
		}
		VECNORMIN(bnorm);
#if ASTEROIDLIST
		glBegin(GL_TRIANGLE_FAN);
		glNormal3dv(bnorm);
		glVertex3dv(bpos);
		for(i = 0; i < 9; i++){
			int i1 = i%8, j = 7, j1 = 8;
			glNormal3dv(norm[i1][6]);
			glVertex3dv(pos[i1][j]);
		}
		glEnd();
#endif
/*		glBegin(GL_LINES);
		glVertex3dv(pos[0][8]);
		{
			avec3_t sum;
			VECADD(sum, pos[0][8], bnorm);
			glVertex3dv(sum);
		}
		glEnd();*/

		VECNULL(tnorm);

		/* top cap */
		for(i = 0; i < 8; i++){
			avec3_t dx, dy, vec;
			int i1 = (i+1)%8;
			VECSUB(dx, pos[i][1], tpos);
			VECSUB(dy, pos[i1][1], tpos);
			VECVP(vec, dy, dx);
			VECNORMIN(vec);
			VECADDIN(tnorm, vec);
		}
		VECNORMIN(tnorm);
#if ASTEROIDLIST
		glBegin(GL_TRIANGLE_FAN);
		glNormal3dv(tnorm);
		glVertex3dv(tpos);
		for(i = 9; i; i--){
			int i1 = i%8, j = 0, j1 = 1;
			glNormal3dv(norm[i1][0]);
			glVertex3dv(pos[i1][j1]);
		}
		glEnd();
#endif
	}
#if ASTEROIDLIST
	glEndList();
#else
	varies[seed] = 1;
#endif
	}
	glPushMatrix();
	glTranslated(org[0], org[1], org[2]);
	gldMultQuat(qrot);
	glScaled(rad / 2, rad / 2, rad / 2);
	glCallList(varies[seed]);
	glPopMatrix();
}



#define RING_TRACKS 16

typedef struct ringVertexData{
	Mat4d matz;
	int relative;
	double arad;
	Vec3d spos;
	Vec3d apos;
	Vec3d vwpos;
	Mat4d matty;
	Mat4d imatty;
	char shadow[RING_TRACKS+2][RING_CUTS];
} ringVertexData;

static void ringVertex3d(double x, double y, double z, COLOR32 col, ringVertexData *rvd, int n, int i){
	Vec3d pos(x, y, z);

	/* i really dont like to trace a ray here just for light source obscuring, but couldnt help. */
	if(rvd->shadow[n][i % RING_CUTS]){
		glColor4ub(COLOR32R(col) / 8, COLOR32G(col) / 8, COLOR32B(col) / 8, COLOR32A(col));
/*		glColor4ub(0, 0, 0, COLOR32A(col));*/
	}
	else{
		gldColor32(col);
	}

	glTexCoord1i(n);

	if(rvd->relative){
		Vec3d pp = rvd->matz.vp3(pos).norm();
		glVertex3dv(pp);
	}
	else{
		glNormal3dv(rvd->imatty.vp3(rvd->vwpos) - pos);
		glVertex3dv(pos);
	}
}

void ring_draw(const Viewer *vw, const struct Astrobj *a, const Vec3d &sunpos, char start, char end, const Quatd &qrot, double thick, double minrad, double maxrad, double t){
	int i, inside = 0;
	double sp, dir;
	double width = (maxrad - minrad) / RING_TRACKS;
	struct random_sequence rs;
	Vec3d delta;
	Mat4d mat;
	Mat4d rotation = qrot.tomat4();
	if(0||start == end)
		return;

	Vec3d apos = vw->cs->tocs(a->pos, a->parent);

	{
		Vec3d v;
		Mat4d mat2;
/*		MAT4ROTX(mat2, mat4identity, pitch);*/
		Mat4d rot = vw->cs->tocsim(a->parent);
		mat = rot * rotation;
/*		mat4mp(mat, rot, mat2);*/
/*		glPushMatrix();
		glLoadIdentity();
		glRotated(10., 1., 0., 0.);
		glGetDoublev(GL_MODELVIEW_MATRIX, mat);
		glPopMatrix();*/
		v = mat.vp3(vec3_001);
		delta = vw->pos - apos;
		sp = delta.sp(v);
		dir = sqrt(delta.slen() - sp * sp) / a->rad; /* diameter can be calculated by sp */
	}

/*	tocs(spos, vw->cs, sun.pos, sun.cs);*/
	Vec3d spos = sunpos;
	Vec3d light = sunpos.norm();

	Vec4<float> light_position;
	Vec3d localight = mat.tdvp3(light);
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthMask(1);
//			LightOn(NULL);
	light_position = localight.cast<float>();
	light_position[3] = 0.;
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, Vec4<float>(.1f, .1f, .1f, 1.f));
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	init_rseq(&rs, (unsigned long)a + 123);

	GLubyte bandtex[RING_TRACKS];
	for(i = 0; i < RING_TRACKS; i++){
		if(i == 0 || i == RING_TRACKS-1)
			bandtex[i] = 0;
		else
			bandtex[i] = rseq(&rs) % 256;
	}

	if(!(minrad < dir && dir < maxrad && -thick < sp && sp < thick)){
		double rad;
		ringVertexData rvd;
		double x, y, roll;
		COLOR32 ocol = COLOR32RGBA(255,255,255,0);
		int cutnum = RING_CUTS;
		double (*cuts)[2];
		int n;
		rvd.vwpos = vw->pos;
		cuts = CircleCuts(cutnum);
		Vec3d xh = mat.vp3(vec3_100);
		x = delta.sp(xh);
		Vec3d yh = mat.vp3(vec3_010);
		y = delta.sp(yh);
		roll = atan2(x, y);

		/* determine whether we are looking the ring from night side */
/*		{
			Vec3d das = spos - apos;
			shade = delta.sp(rotation.vec3(2)) * das.sp(rotation.vec3(2)) < 0.;
		}*/

		glPushMatrix();
		if(vw->relative)
			glLoadIdentity();
		glScaled(1. / a->rad, 1. / a->rad, 1. / a->rad);
		glTranslated(-vw->pos[0], -vw->pos[1], -vw->pos[2]);

#if 1
		rvd.matty = (Mat4d(mat4_u).translatein(apos) * mat).rotz(-roll) * Mat4d(Vec3d(a->rad, 0, 0), Vec3d(0, a->rad, 0), Vec3d(0, 0, a->rad));
		rvd.imatty = Mat4d(Vec3d(1. / a->rad, 0, 0), Vec3d(0, 1. / a->rad, 0), Vec3d(0, 0, 1. / a->rad)).rotz(roll) * (qrot.cnj() * vw->cs->tocsq(a->parent)).tomat4() * Mat4d(mat4_u).translatein(-apos);
#else
		glPushMatrix();
		glLoadIdentity();
		glTranslated(apos[0], apos[1], apos[2]);
		glMultMatrixd(mat);
/*		glRotated(deg_per_rad * pitch, 1., 0., 0.);*/
		glRotated(-deg_per_rad * roll, 0., 0., 1.);
		glScaled(a->rad, a->rad, a->rad);
		glGetDoublev(GL_MODELVIEW_MATRIX, rvd.matty);
		glPopMatrix();
#endif
		glMultMatrixd(rvd.matty);

		rvd.relative = vw->relative;
		rvd.arad = a->rad;
		rvd.apos = apos;
		rvd.spos = spos;

		if(vw->relative){
			glGetDoublev(GL_MODELVIEW_MATRIX, rvd.matz);
			glPopMatrix();
			glPushMatrix();
		}

		for(n = 0; n <= RING_TRACKS+1; n++){
			int i2;
			rad = minrad + (maxrad - minrad) * n / (RING_TRACKS+1);
			for(i = start; i <= end; i++) for(i2 = 0; i2 < 2; i2++){
				int i1 = (i2 ? cutnum - i : i) % cutnum;
				Vec3d rpos, pos, ray;
				pos[0] = cuts[i1][0] * rad;
				pos[1] = cuts[i1][1] * rad;
				pos[2] = 0.;
				rpos = rvd.matty.vp3(pos);
				ray = rpos - spos;
				rvd.shadow[n][i1] = 0/*jHitSphere(apos, a->rad, spos, ray, 1.)*/;
			}
		}

		glEnable(GL_TEXTURE_1D);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_ALPHA, RING_TRACKS, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bandtex);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
/*		for(n = 0; n < RING_TRACKS+1; n++)*/{
			double radn = maxrad;
			double rad = minrad;
			COLOR32 col, ocold, cold;
//			rad = minrad + (maxrad - minrad) * n / (RING_TRACKS+1);
//			radn = minrad + (maxrad - minrad) * (n+1) / (RING_TRACKS+1);
			ocol = COLOR32RGBA(127,127,127,127);
			col = COLOR32RGBA(127,127,127,127);
			glBegin(GL_QUAD_STRIP);
			for(i = start; i <= end; i++){
				int i1 = i % cutnum;
/*				gldColor32(ocol);*/
				ringVertex3d(cuts[i1][0] * rad, cuts[i1][1] * rad, 0., ocol, &rvd, 0, i1);
/*				gldColor32(col);*/
				ringVertex3d(cuts[i1][0] * radn, cuts[i1][1] * radn, 0., col, &rvd, 1, i1);
			}
			glEnd();
			glBegin(GL_QUAD_STRIP);
			for(i = cutnum - end; i <= cutnum - start; i++){
				int i1 = i % cutnum;
/*				gldColor32(ocol);*/
				ringVertex3d(cuts[i1][0] * rad, cuts[i1][1] * rad, 0., ocol, &rvd, 0, i1);
/*				gldColor32(col);*/
				ringVertex3d(cuts[i1][0] * radn, cuts[i1][1] * radn, 0., col, &rvd, 1, i1);
			}
			glEnd();
			ocol = col;
			rad = radn;
		}
		glDisable(GL_TEXTURE_1D);
		glPopMatrix();
	}
	else
		inside = 1;
	/* so close to the ring that its particles can be viewed */
	if(minrad < dir && dir < maxrad && -thick - .01 * (1<<6) < sp && sp < thick + .01 * (1<<6)){
		int inten, ointen;
		glPushMatrix();
		glMultMatrixd(rotation);
		{
			double rad;
			double f;
			for(rad = minrad; rad < maxrad;){
				double radn;
				radn = rad + width;
				inten = radn < maxrad ? rseq(&rs) % 256 : 0;
				if(dir < rad)
					break;
				ointen = inten;
				rad = radn;
			}

			/* linear interpolation */
			f = (dir - rad + width) / width;
			inten = (inten * f + ointen * (1 - f));
		}
		if(inside){
			double sign = sp / fabs(sp);
			double y, z;
			double (*cuts)[2];
			inside = 1;
			cuts = CircleCuts(16);
			y = sqrt(1 - sp * sp / thick / thick);
			z = sqrt(1 - y * y);
	/*		glRotated(deg_per_rad * pitch, 1., 0., 0.);*/
			{
				Vec3d v;
				GLubyte bri;
				glBegin(GL_QUAD_STRIP);
				for(i = 0; i <= 16; i++){
					int i1 = i % 16;
					v[0] = cuts[i1][0];
					v[1] = cuts[i1][1];
					v[2] = 0.;
					bri = 32 + (-v.sp(localight) + 1.) * .5 * 127;
					glColor4ub(bri,bri,bri,inten);
					glVertex3d(cuts[i1][0], cuts[i1][1], 0.);
					v[0] = cuts[i1][0] * z;
					v[1] = cuts[i1][1] * z;
					v[2] = sign * y;
					bri = 32 + (-v.sp(localight) + 1.) * .5 * 127;
					glColor4ub(bri,bri,bri,0);
		/*			glVertex3d(cuts[i1][0], cuts[i1][1], thick / sp - sign);*/
					glVertex3d(cuts[i1][0] * z, cuts[i1][1] * z, sign * y);
				}
				glEnd();
				glBegin(GL_TRIANGLE_FAN);
				v[0] = 0.;
				v[1] = 0.;
				v[2] = -sign;
				bri = 32 + (-v.sp(localight) + 1.) * .5 * 127;
				glColor4ub(bri,bri,bri, sp * sign / thick * inten / 2);
				glVertex3d(0., 0., -sign);
				for(i = 0; i <= 16; i++){
					int i1 = i % 16;
					v[0] = cuts[i1][0];
					v[1] = cuts[i1][1];
					v[2] = 0.;
					bri = 32 + (-v.sp(localight) + 1.) * .5 * 127;
					glColor4ub(bri,bri,bri,inten);
					glVertex3d(cuts[i1][0], cuts[i1][1], 0.);
				}
				glEnd();
			}
		}
		glDisable(GL_COLOR_MATERIAL);
		if(start == 0)
		{
			int i, count = inten / 4, total = 0;
			struct random_sequence rs;
/*			timemeas_t tm;*/
			const double destvelo = .01; /* destination velocity */
			const double width = .00002 * 1000. / vw->vp.m; /* line width changes by the viewport size */
			double plpos[3];
//			Vec4<float> light_position /*= { sunpos[0], sunpos[1], sunpos[2], 0.0 }*/;
			Vec3d nh0(0., 0., -1.), nh;
			Vec3d vwpos;
			Mat4d mat;
			Mat4d proj;
			int level;
			vwpos = rotation.tdvp3(delta);
/*			TimeMeasStart(&tm);*/
			{
				Mat4d modelmat, persmat;
				glGetDoublev(GL_MODELVIEW_MATRIX, modelmat);
				glGetDoublev(GL_PROJECTION_MATRIX, persmat);
				mat = persmat * modelmat;
			}
			nh = vw->irot.vp3(nh0);
			glPushMatrix();
/*			glLoadMatrixd(vw->rot);*/
		/*	glTranslated(-(*view)[0], -(*view)[1], -(*view)[2]);*/

			/* restore original projection matrix temporarily to make depth
			  buffer make sense. 'relative' projection frustum has near clipping
			  plane with so small z value that the precision drops so hard. */
			glGetDoublev(GL_PROJECTION_MATRIX, proj);
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);

			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			for(level = 6; level; level--){
				double cellsize, rocksize = (1<<level) * .0001;
				double rotspeed = .1 / level;
				Vec3d disp;
				Vec3d plpos;
				plpos = vw->pos;
				plpos += nh * -rocksize;
				init_rseq(&rs, level + 23342);
				cellsize = .01 * (1<<level);
				if(level == 6){
					glPushAttrib(GL_POINT_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
					glDisable(GL_LIGHTING);
					glDisable(GL_POINT_SMOOTH);
					glPointSize(2);
					glBegin(GL_POINTS);
				}
				for(i = level == 6 ? count * 64 : count; i; i--){
					double pos[3], pyr[3], x0, x1, z0, z1, base;
					unsigned long red;
					pos[0] = vwpos[0] + (drseq(&rs) - .5) * 2 * cellsize;
					pos[0] = /*pl.pos[0] +*/ +(floor(pos[0] / cellsize) * cellsize + cellsize / 2. - pos[0]);
					pos[1] = vwpos[1] + (drseq(&rs) - .5) * 2 * cellsize;
					pos[1] = floor(pos[1] / cellsize) * cellsize + cellsize / 2 - pos[1];
					pos[2] = vwpos[2] + (drseq(&rs) - .5) * 2 * cellsize;
					pos[2] = /*pl.pos[2] +*/ +(floor(pos[2] / cellsize) * cellsize + cellsize / 2. - pos[2]);
					Quatd qrot;
					for(int j = 0; j < 4; j++)
						qrot[j] = drseq(&rs);
					Vec3d omg;
					for(int j = 0; j < 3; j++)
						omg[j] = rotspeed * t;
					qrot = qrot.quatrotquat(omg);
					red = rseq(&rs);
		/*			if((-1. < pos[0] && pos[0] < 1. && -1. < pos[2] && pos[2] < 1.
						? -.01 < pos[0] && pos[0] < .01
						: (pos[2] < -.01 || .01 < pos[2])))
						continue;*/
					if(pos[2] + vwpos[2] < -thick || thick < pos[2] + vwpos[2])
						continue;
			#if 1
					{
						avec4_t dst;
						avec4_t vec;
						VECCPY(vec, pos);
						VECSADD(vec, nh, rocksize / vw->fov);
						MAT4VP3(dst, mat, vec);
						if(dst[2] < 0. || dst[0] < -dst[2] || dst[2] < dst[0] || dst[1] < -dst[2] || dst[2] < dst[1])
							continue;
					}
			#endif
					if(level == 6){
						int br;
						br = 128 * (1. - VECSP(pos, localight) / VECLEN(pos)) / 2.;
						br = br < 0 ? 0 : br;
						glColor3ub(br, br, br);
						glVertex3dv(pos);
					}
					else
						drawasteroid(pos, qrot, rocksize, i + level * count);
					total++;
				}
				if(level == 6){
					glEnd();
					glPopAttrib();
				}
			}
			glPopMatrix();
//			LightOff();

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixd(proj);
			glMatrixMode(GL_MODELVIEW);

/*			fprintf(stderr, "ring[%3d]: %lg sec\n", total, TimeMeasLap(&tm));*/
		}
		glPopMatrix();
	}
	glPopAttrib();
}
