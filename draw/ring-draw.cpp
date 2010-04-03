#include "../astrodraw.h"
#include "../judge.h"
#include "../astrodef.h"
#include "../Universe.h"
extern "C"{
//#include <clib/amat3.h>
#include <clib/rseq.h>
#include <clib/timemeas.h>
//#include <clib/gl/cull.h>
#include <clib/gl/gldraw.h>
#include <clib/gl/multitex.h>
#include <clib/c.h>
#include <clib/mathdef.h>
}
#include <cpplib/gl/cullplus.h>
#include <gl/glext.h>

#define SQRT2P2 (M_SQRT2/2)

#define ASTEROIDLIST 1


extern float g_astro_ambient;


static void drawasteroid(const Vec3d &org, const Quatd &qrot, double rad, unsigned long seed){
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
	Vec3d *pos[8][9]; /* indirect vector referencing to simplify normal generation */
#if ASTEROIDLIST
	Vec3d posv[8][7], norm[8][8];
	Vec3d tpos, tnorm;
	Vec3d bpos, bnorm;
	glNewList(varies[seed] = glGenLists(1), GL_COMPILE);
	{
		GLfloat dif[] = {.5, .475f, .45f, 1.}, amb[] = {.1f, .1f, .05f, 1.};
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	}
#else
	double (*posv)[8][3] = caches[seed].pos, (*norm)[8][3] = caches[seed].norm;
	double *tpos = caches[seed].tpos, *tnorm = caches[seed].tnorm;
	double *bpos = caches[seed].bpos, *bnorm = caches[seed].bnorm;
#endif
	for(i = 0; i < 8; i++){
		pos[i][0] = &tpos;
		for(j = 1; j < 8; j++)
			pos[i][j] = &posv[i][j-1];
		pos[i][8] = &bpos;
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
		(*pos[i][j])[0] = (drseq(&rs) + 1.) * ci * sj;
		(*pos[i][j])[1] = (drseq(&rs) + 1.) * cj;
		(*pos[i][j])[2] = (drseq(&rs) + 1.) * si * sj;
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
#if 1
		Vec3d dx, dy, vec;
		dx = *pos[i1][j] - *pos[i][j];
		dy = *pos[i][j1] - *pos[i][j];
		norm[i][j0] = dx.vp(dy).normin();
		dy = *pos[i][jn] - *pos[i][j];
		norm[i][j0] += dy.vp(dx).normin();
		dx = *pos[in][j] - *pos[i][j];
		norm[i][j0] += dx.vp(dy).normin();
		dy = *pos[i][j1] - *pos[i][j];
		norm[i][j0] += dy.vp(dx).normin();
		norm[i][j0].normin();
#elif 0
		Vec3d dx, dy, vec;
		dx = *pos[i1][j] - *pos[i][j];
		dy = *pos[i][j1] - *pos[i][j];
		norm[i][j0] = dx.vp(dy).norm();
		dy = *pos[i][jn] - *pos[i][j];
		norm[i][j0] += dy.vp(dx).norm();
		dx = *pos[in][j] - *pos[i][j];
		norm[i][j0] += dx.vp(dy).norm();
		dy = *pos[i][j1] - *pos[i][j];
		norm[i][j0] += dy.vp(dx).norm();
		norm[i][j0].normin();
#else
		Vec3d dx, dy, vec;
		VECSUB(dx, *pos[i1][j], *pos[i][j]);
		VECSUB(dy, *pos[i][j1], *pos[i][j]);
		VECVP(vec, dx, dy);
		VECNORMIN(vec);
		VECCPY(norm[i][j0], vec);
		VECSUB(dy, *pos[i][jn], *pos[i][j]);
		VECVP(vec, dy, dx);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECSUB(dx, *pos[in][j], *pos[i][j]);
		VECVP(vec, dx, dy);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECSUB(dy, *pos[i][j1], *pos[i][j]);
		VECVP(vec, dy, dx);
		VECNORMIN(vec);
		VECADDIN(norm[i][j0], vec);
		VECNORMIN(norm[i][j0]);
#endif
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
		glVertex3dv(*pos[i][j]);
		glNormal3dv(norm[i1][j0]);
		glVertex3dv(*pos[i1][j]);
		glNormal3dv(norm[i][j01]);
		glVertex3dv(*pos[i][j1]);
		glNormal3dv(norm[i1][j01]);
		glVertex3dv(*pos[i1][j1]);
		glEnd();
	}
	{
		bnorm.clear();

		/* bottom cap */
		for(i = 0; i < 8; i++){
			int i1 = (i+1)%8;
			bnorm += (*pos[i][7] - bpos).vp(*pos[i1][7] - bpos).normin();
		}
		bnorm.normin();
#if ASTEROIDLIST
		glBegin(GL_TRIANGLE_FAN);
		glNormal3dv(bnorm);
		glVertex3dv(bpos);
		for(i = 0; i < 9; i++){
			int i1 = i%8, j = 7, j1 = 8;
			glNormal3dv(norm[i1][6]);
			glVertex3dv(*pos[i1][j]);
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

		tnorm.clear();

		/* top cap */
		for(i = 0; i < 8; i++){
			int i1 = (i+1)%8;
			tnorm += (*pos[i1][1] - tpos).vp(*pos[i][1] - tpos).normin();
		}
		tnorm.normin();
#if ASTEROIDLIST
		glBegin(GL_TRIANGLE_FAN);
		glNormal3dv(tnorm);
		glVertex3dv(tpos);
		for(i = 9; i; i--){
			int i1 = i%8, j = 0, j1 = 1;
			glNormal3dv(norm[i1][0]);
			glVertex3dv(*pos[i1][j1]);
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

static void ring_draw_cell(double l, double m, double radial_divides, int count, const double cellsizes[3],
						   const Vec3d &vwpos, const Vec3d &vwapos, const Mat4d &mrot, const GLcull &glc,
						   double thick, const Vec3d &light, const Vec3d &localight, double gtime, int &total, bool inshadow)
{
	for(int level = 6; level; level--){
		const double cellsize = .01 * (1<<level);
		const double rocksize = (1<<level) * .0001;
		double rotspeed = .5 / level;
		random_sequence rs;
		init_rseqf_double(&rs, (level + 23342) + l * 3869723 + floor(m * radial_divides) / radial_divides);
		if(level == 6){
			glPushAttrib(GL_POINT_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_POINT_SMOOTH);
			glPointSize(2);
			glBegin(GL_POINTS);
		}
		else{
			glPushAttrib(GL_POLYGON_BIT);
			glEnable(GL_CULL_FACE);
		}
		int i = (level == 6 ? count * 32 : count) + 2;
		for(; i; i--){
			Vec3d pos;
			for(int j = 0; j < 3; j++)
				pos[j] = (drseq(&rs)) * cellsizes[j];
			pos[2] += vwpos[2];
			pos[2] = pos[2] - ::floor(pos[2] / cellsize) * cellsize - cellsize / 2.;
			pos[2] -= vwpos[2];
			pos = mrot.vp3(pos);

			// Initial rotation
			Quatd qrot;
			for(int j = 0; j < 4; j++)
				qrot[j] = drseq(&rs);
			qrot.normin();

			// Angular velocity vector
			Vec3d omg;
			for(int j = 0; j < 3; j++)
				omg[j] = drseq(&rs);

			double angle = drseq(&rs) * rotspeed * gtime / 2.;

			if(glc.cullFrustum(pos, rocksize))
				continue;
			if(pos[2] + vwapos[2] < -thick || thick < pos[2] + vwapos[2])
				continue;
			if(level == 6 || rocksize * glc.scale(pos) < 5.){
				if(level != 6){
					glPushAttrib(GL_POINT_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
					glDisable(GL_LIGHTING);
					glDisable(GL_POINT_SMOOTH);
					glPointSize(2);
					glBegin(GL_POINTS);
				}
				GLfloat br;
				br = inshadow ? .5f * g_astro_ambient : .5f * float((1. - pos.norm().sp(light)) / 2. + g_astro_ambient);
				br = br < 0 ? 0 : br;
				glColor3f(br, br, br);
				glVertex3dv(pos);
				if(level != 6){
					glEnd();
					glPopAttrib();
				}
			}
			else{
				Quatd omgdt = Quatd(omg.norm() * sin(angle)) + Quatd(0,0,0,cos(angle));
				qrot *= omgdt;

				drawasteroid(pos, qrot, rocksize, i + level * count);
			}
			total++;
		}
		if(level == 6)
			glEnd();
		glPopAttrib();
	}
}


#define RING_TRACKS 16

typedef struct ringVertexData{
	Mat4d matz;
	int relative;
	Vec3d vwpos;
	Mat4d matty;
	Mat4d imatty;
} ringVertexData;

static void ringVertex3d(double x, double y, double z, COLOR32 col, ringVertexData *rvd, int n, int i){
	Vec3d pos(x, y, z);

	gldColor32(col);

	glTexCoord1i(n);
	if(glMultiTexCoord2dARB)
		glMultiTexCoord2dARB(GL_TEXTURE1_ARB, x, y);

	if(rvd->relative){
		Vec3d pp = rvd->matz.vp3(pos).norm();
		glVertex3dv(pp);
	}
	else{
		glNormal3dv(rvd->imatty.vp3(rvd->vwpos) - pos);
		glVertex3dv(pos);
	}
}

static void ring_setshadow(const Quatd &q, double angle, double ipitch){
	static GLuint texname = 0;
	static GLubyte texambient = 0;
	GLubyte ambient = GLubyte(g_astro_ambient * 255.f);
	glActiveTextureARB(GL_TEXTURE1_ARB);

	// Regenerate texture if astronomical ambient value is changed.
	if(!texname || ambient != texambient){
		const int shadowtexsize = 256;
		static GLubyte teximg[shadowtexsize][shadowtexsize] = {0};
		int x, y;
		for(y = 0; y < shadowtexsize; y++) for(x = 0; x < shadowtexsize; x++){
			double dx = double(x - shadowtexsize / 2) / (shadowtexsize / 2), dy = double(y - shadowtexsize / 2) / (shadowtexsize / 2);
			teximg[y][x] = (dx * dx + dy * dy) < 1. ? ambient : 255;
		}
		glGenTextures(1, &texname);
		glBindTexture(GL_TEXTURE_2D, texname);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, shadowtexsize, shadowtexsize, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, teximg);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		GLfloat one = 1.f;
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &one);
		texambient = ambient;
	}
	glBindTexture(GL_TEXTURE_2D, texname);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glTranslated(+.5, +.5, 0.);
	glScaled(.5, .5 * ipitch, 1.); // Scale x by inverse of 2 for texture coordinates unit is center object's radius, not diameter.
	glRotated(angle * deg_per_rad, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void ring_draw(const Viewer *vw, const Astrobj *a, const Vec3d &sunpos, char start, char end, const Quatd &qrot, double thick, double minrad, double maxrad, double t, double oblateness){
	if(0||start == end)
		return;
	int i, inside = 0;
	double sp, dir;
	double width = (maxrad - minrad) / RING_TRACKS;
	struct random_sequence rs;
	Vec3d delta;
	Mat4d mat;
	Quatd axisrot = Quatd::direction(qrot.trans(vec3_001));
	Vec3d vwapos = axisrot.cnj().trans(a->parent->tocs(vw->pos, vw->cs) - a->pos);
	double phase = atan2(-vwapos[0], vwapos[1]) + M_PI;
	double r = ::sqrt(vwapos[0] * vwapos[0] + vwapos[1] * vwapos[1]);
	Quatd qphase(0, 0, sin(phase / 2.), cos(phase / 2.));
	Quatd qintrot = axisrot * qphase;
	Mat4d rotation = qintrot.tomat4();

	Vec3d sunapos = axisrot.cnj().trans(sunpos);
	double sunphase = atan2(-sunapos[0], sunapos[1]);

	Vec3d apos = vw->cs->tocs(a->pos, a->parent);

	{
		Vec3d v;
		Mat4d mat2;
		Mat4d rot = vw->cs->tocsim(a->parent);
		mat = rot * rotation;
/*		mat4mp(mat, rot, mat2);*/
		v = mat.vp3(vec3_001);
		delta = vw->pos - apos;
		sp = delta.sp(v);
		dir = sqrt(delta.slen() - sp * sp) / a->rad; /* diameter can be calculated by sp */
	}

/*	tocs(spos, vw->cs, sun.pos, sun.cs);*/
	Vec3d spos = sunpos;
	Vec3d light = sunpos.norm();

	Vec4<float> light_position;
	Vec3d localight = /*mat.tdvp3*/(light);
//	Vec3d localight = qintrot.trans(light);
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
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
		ringVertexData rvd;
		double roll;
		int cutnum = RING_CUTS;
		double (*cuts)[2];
		rvd.vwpos = vw->pos;
		cuts = CircleCuts(cutnum);
		{
			double x, y;
			Vec3d xh = mat.vp3(vec3_100);
			x = delta.sp(xh);
			Vec3d yh = mat.vp3(vec3_010);
			y = delta.sp(yh);
			roll = atan2(x, y);
		}

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

		rvd.matty = (Mat4d(mat4_u).translatein(apos) * mat).rotz(-roll) * Mat4d(Vec3d(a->rad, 0, 0), Vec3d(0, a->rad, 0), Vec3d(0, 0, a->rad));
		rvd.imatty = Mat4d(Vec3d(1. / a->rad, 0, 0), Vec3d(0, 1. / a->rad, 0), Vec3d(0, 0, 1. / a->rad)).rotz(roll) * (qrot.cnj() * vw->cs->tocsq(a->parent)).tomat4() * Mat4d(mat4_u).translatein(-apos);

		glMultMatrixd(rvd.matty);

		rvd.relative = vw->relative;
		Vec3d intspos = qintrot.cnj().trans(spos);

		if(vw->relative){
			glGetDoublev(GL_MODELVIEW_MATRIX, rvd.matz);
			glPopMatrix();
			glPushMatrix();
		}

		glEnable(GL_TEXTURE_1D);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_ALPHA, RING_TRACKS, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bandtex);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		if(glActiveTextureARB){
			ring_setshadow(qintrot, phase - sunphase, fabs(sunapos[2] / sunapos.len() / (1. - oblateness)));
		}
		{
			double radn = maxrad;
			double rad = minrad;
			COLOR32 col = COLOR32RGBA(127,127,127,127);
			bool shadowing = true;
			glBegin(GL_QUADS);
			for(i = start; i < end; i++){
				int i1 = (i+1) % cutnum;
				Vec3d fanpos = Vec3d(cuts[i][0] * radn, cuts[i][1] * radn, 0.);
				bool thisshadowing = 0 < fanpos.sp(intspos);
				if(thisshadowing != shadowing && glActiveTextureARB){
					glEnd();
					glActiveTextureARB(GL_TEXTURE1_ARB);
					(thisshadowing ? glEnable : glDisable)(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
					shadowing = thisshadowing;
					glBegin(GL_QUADS);
				}
				ringVertex3d(cuts[i][0] * radn, cuts[i][1] * radn, 0., col, &rvd, 1, i);
				ringVertex3d(cuts[i][0] * rad, cuts[i][1] * rad, 0., col, &rvd, 0, i);
				ringVertex3d(cuts[i1][0] * rad, cuts[i1][1] * rad, 0., col, &rvd, 0, i1);
				ringVertex3d(cuts[i1][0] * radn, cuts[i1][1] * radn, 0., col, &rvd, 1, i1);
			}
			glEnd();
			glBegin(GL_QUADS);
			if(1)for(i = cutnum - end; i < cutnum - start; i++){
				int i1 = (i+1) % cutnum;
				Vec3d fanpos = Vec3d(cuts[i][0] * radn, cuts[i][1] * radn, 0.);
				bool thisshadowing = 0 < fanpos.sp(intspos);
				if(thisshadowing != shadowing && glActiveTextureARB){
					glEnd();
					glActiveTextureARB(GL_TEXTURE1_ARB);
					(thisshadowing ? glEnable : glDisable)(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
					shadowing = thisshadowing;
					glBegin(GL_QUADS);
				}
				ringVertex3d(cuts[i][0] * radn, cuts[i][1] * radn, 0., col, &rvd, 1, i);
				ringVertex3d(cuts[i][0] * rad, cuts[i][1] * rad, 0., col, &rvd, 0, i);
				ringVertex3d(cuts[i1][0] * rad, cuts[i1][1] * rad, 0., col, &rvd, 0, i1);
				ringVertex3d(cuts[i1][0] * radn, cuts[i1][1] * radn, 0., col, &rvd, 1, i1);
			}
			glEnd();
		}
		glDisable(GL_TEXTURE_1D);
		if(glActiveTextureARB){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		glPopMatrix();
	}
	else
		inside = 1;
	/* so close to the ring that its particles can be viewed */
	if(minrad < dir && dir < maxrad && -thick - .01 * (1<<6) < sp && sp < thick + .01 * (1<<6)){
		GLfloat inten;
		Vec3d oblate_vwpos = vwapos;
		oblate_vwpos[2] /= 1. - oblateness;
		Vec3d oblate_spos = sunapos;
		oblate_spos[2] /= 1. - oblateness;
		Vec3d viewsun = oblate_spos - oblate_vwpos;
		bool inshadow = jHitSphere(vec3_000, a->rad, oblate_vwpos, viewsun, 1.);
		glPushMatrix();
		gldMultQuat(axisrot);
		{
			double rad;
			GLfloat ointen;
			for(rad = minrad; rad < maxrad;){
				double radn;
				radn = rad + width;
				inten = radn < maxrad ? (rseq(&rs) % 256) / 255.f : 0;
				if(dir < rad)
					break;
				ointen = inten;
				rad = radn;
			}

			/* linear interpolation */
			double f = (dir - rad + width) / width;
			inten = GLfloat(inten * f + ointen * (1 - f));
		}

		// Ring shading
		if(inside){
			double sign = sp / fabs(sp);
			double y, z;
			double (*cuts)[2];
			inside = 1;
			cuts = CircleCuts(16);
			y = sqrt(1 - sp * sp / thick / thick);
			z = sqrt(1 - y * y);
			Vec3d v;
			GLfloat ambient = (g_astro_ambient * .5f);
			GLfloat bri;
			glBegin(GL_QUAD_STRIP);
			for(i = 0; i <= 16; i++){
				int i1 = i % 16;
				v[0] = cuts[i1][0];
				v[1] = cuts[i1][1];
				v[2] = 0.;
				bri = inshadow ? ambient : ambient + GLfloat((-v.sp(localight) + 1.) * .5 * .5);
				glColor4f(bri,bri,bri,inten);
				glVertex3d(cuts[i1][0], cuts[i1][1], 0.);
				v[0] = cuts[i1][0] * z;
				v[1] = cuts[i1][1] * z;
				v[2] = sign * y;
				bri = inshadow ? ambient : ambient + GLfloat((-v.sp(localight) + 1.) * .5 * .5);
				glColor4f(bri,bri,bri,0);
//				glVertex3d(cuts[i1][0], cuts[i1][1], thick / sp - sign);
				glVertex3d(cuts[i1][0] * z, cuts[i1][1] * z, sign * y);
			}
			glEnd();
			glBegin(GL_TRIANGLE_FAN);
			v[0] = 0.;
			v[1] = 0.;
			v[2] = -sign;
			bri = inshadow ? ambient : ambient + GLfloat((-v.sp(localight) + 1.) * .5 * .5);
			glColor4f(bri,bri,bri, GLfloat(sp * sign / thick * inten / 2));
			glVertex3d(0., 0., -sign);
			for(i = 0; i <= 16; i++){
				int i1 = i % 16;
				v[0] = cuts[i1][0];
				v[1] = cuts[i1][1];
				v[2] = 0.;
				bri = inshadow ? ambient : ambient + GLfloat((-v.sp(localight) + 1.) * .5 * .5);
				glColor4f(bri,bri,bri,inten);
				glVertex3d(cuts[i1][0], cuts[i1][1], 0.);
			}
			glEnd();
		}

		glDisable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHTING);
		if(start == 0){
			const double radial_divides = 1000000;
			const double radial = 2 * M_PI / radial_divides;
			const double concentrical = .5;
			const int mcells = 5, lcells = 5;
			double gtime = 0.;
			{
				CoordSys *top = a->parent;
				for(; top->parent; top = top->parent);
				if(!top || !top->toUniverse());
				else
					gtime = top->toUniverse()->astro_time;
			}

			/* restore original projection matrix temporarily to make depth
			  buffer make sense. 'relative' projection frustum has near clipping
			  plane with so small z value that the precision drops so hard. */
			Mat4d proj;
			glGetDoublev(GL_PROJECTION_MATRIX, proj);
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);

			GLcull glc(vw->gc->getFov(), vec3_000, vw->irot, vw->gc->getNear(), vw->gc->getFar());

			double lbase = floor(r / concentrical);
			for(double l = lbase - lcells / 2 + 1; l < lbase + (lcells + 1) / 2 + 1; l++){
				double dist = l * concentrical;
				double omega = ::sqrt(UGC * a->mass / dist) / dist; // orbital speed at cell
				double angle = omega * gtime;
				double mbase = floor((phase - angle) / radial);
				for(double m = mbase - mcells / 2; m < mbase + (mcells + 1) / 2; m++){
					int count = int(inten * 256.f / 4 / 16), total = 0;
		/*			timemeas_t tm;*/
					double cellangle = omega * gtime;
					Quatd qangle0(0,0,sin(angle/2.),cos(angle/2.));
					Quatd qangle = axisrot * qangle0;
					double cellphase = m * radial + cellangle;
					Vec3d vwpos = Vec3d(l * concentrical * sin(cellphase), -l * concentrical * cos(cellphase), 0) - vwapos;

					glPushMatrix();
					Mat4d mpos = mat4_u.translate(vwpos);
					Mat4d mrot = mpos.rotz(cellphase);
#ifndef NDEBUG
					glPushMatrix();
					glMultMatrixd(mrot);
					glScaled(r * radial, concentrical, 2.);
					glTranslated(0., 0., -.5);
					glPushAttrib(GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_LIGHTING);
					glDisable(GL_POLYGON_SMOOTH);
					glColor4ub(255,0,0,255);
					glBegin(GL_LINES);
					{
						int i, j, k;
						for(i = 0; i < 3; i++) for(j = 0; j <= 1; j++) for(k = 0; k <= 1; k++){
							double v[3];
							v[i] = j;
							v[(i+1)%3] = k;
							v[(i+2)%3] = 0.;
							glVertex3dv(v);
							v[(i+2)%3] = 1.;
							glVertex3dv(v);
						}
					}
					glEnd();
					glPopAttrib();
					glPopMatrix();
#endif

					glEnable(GL_DEPTH_TEST);
					glDisable(GL_BLEND);
					glLightfv(GL_LIGHT0, GL_DIFFUSE, inshadow ? Vec4<float>(g_astro_ambient/2, g_astro_ambient/2, g_astro_ambient/2, 1.f) : Vec4<float>(1.f, 1.f, 1.f, 1.f));
					double cellsizes[3] = {r * radial, concentrical, 1.};
					ring_draw_cell(l, m - floor(m / radial_divides) * radial_divides, radial_divides, count, cellsizes, vwpos, vwapos, mrot, glc, thick, light, localight, gtime, total, inshadow);
					glPopMatrix();

		/*			fprintf(stderr, "ring[%3d]: %lg sec\n", total, TimeMeasLap(&tm));*/
				}
			}
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixd(proj);
			glMatrixMode(GL_MODELVIEW);
		}
		glPopMatrix();
	}
	glPopAttrib();
}
