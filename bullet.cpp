#include "bullet.h"
#include "coordsys.h"
#include "astro.h"
#include "player.h"
#include "judge.h"
//#include "warutil.h"
extern "C"{
#include <clib/amat4.h>
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/colseq/cs.h>
#include <clib/sound/wavemixer.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/gldraw.h>
}
#include <GL/glext.h>
#include <limits.h>
#include <stddef.h>


const char *Bullet::idname()const{
	return "bullet";
}

const char *Bullet::classname()const{
	return "Bullet";
}

#if 0
void sufmodel_normalize(sufmodel_t *mdl);

/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_fireburn[] = {
	{0.03, COLOR32RGBA(255, 255, 255, 255)},
	{0.035, COLOR32RGBA(255, 255, 128, 255)},
	{0.035, COLOR32RGBA(128, 32, 32, 127)},
};
static const struct color_sequence cs_fireburn = DEFINE_COLSEQ(cnl_fireburn, (COLORREF)-1, 0.1);



typedef struct bullet bullet_t;



static void bullet_draw(struct bullet *, wardraw_t*);
static void bullet_drawmodel(struct bullet *pb, wardraw_t *wd){pb,wd;}
static int bullet_is_bullet(bullet_t *p){
	return 1;
}

static struct bullet_static bullet_s = {
	{NULL},
	bullet_anim,
	bullet_draw,
	bullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
};


static void nbullet_draw(struct bullet *pb, wardraw_t *wd);
static void nbullet_drawmodel(struct bullet *pb, wardraw_t *wd);
static void sbullet_drawmodel(struct bullet *pb, wardraw_t *wd);
static void mortarhead_drawmodel(struct bullet *pt, wardraw_t *wd);
static int shrapnelshell_anim(struct bullet *, warf_t *, struct tent3d_line_list *, double dt);
static void APFSDS_draw(struct bullet *pb, wardraw_t *wd);
static void APFSDS_drawmodel(struct bullet *pb, wardraw_t *wd);
static int bulletswarm_anim(struct bullet *, warf_t *, struct tent3d_line_list *, double dt);
static void bulletswarm_draw(struct bullet *, wardraw_t*);
static void bulletswarm_drawmodel(struct bullet *pb, wardraw_t *wd);

static struct bullet_static NormalBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	nbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, ShotgunBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	sbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, MortarHead_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	mortarhead_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, TracerBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	nbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, ExplosiveBullet_s = {
	{NULL},
	bullet_anim,
	nbullet_draw,
	nbullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, ShrapnelShell_s = {
	{NULL},
	shrapnelshell_anim,
	bullet_draw,
	bullet_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, APFSDS_s = {
	{NULL},
	bullet_anim,
	nbullet_draw/*APFSDS_draw*/,
	APFSDS_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
}, BulletSwarm_s = {
	{NULL},
	bulletswarm_anim,
	bulletswarm_draw,
	bulletswarm_drawmodel,
	bullet_postframe,
	bullet_get_homing_target,
	bullet_is_bullet,
};

typedef struct NormalBullet{
	bullet_t st;
	double rad;
} NormalBullet;

typedef struct TracerBullet{
	bullet_t st;
	double rad;
} TracerBullet;

typedef struct BulletSwarm{
	bullet_t st;
	double rad; /* Radius of bulletswarm */
	double vrad; /* Velocity of radius expansion */
	int count;
	unsigned baf[1]; /* Bullet Availability Flags; extensible array */
} BulletSwarm;

static void BulletInit(struct bullet *pb, warf_t *w, struct bullet_static *vft){
	pb->active = 1;
	pb->grav = 1;
	pb->vft = vft;
	if(w){
		pb->next = w->bl;
		w->bl = pb;
	}
	else
		pb->next = NULL;
	VECNULL(pb->pos);
	VECNULL(pb->velo);
	VECNULL(pb->pyr);
	VECNULL(pb->omg);
	QUATIDENTITY(pb->rot);
	pb->mass = 1.;
	pb->moi = 1.;
	pb->health = 1.;
	pb->w = w;
	pb->enemy = NULL;
	pb->race = pb->shoots = pb->shoots2 = pb->kills = pb->deaths = 0;
	pb->inputs.change = pb->inputs.press = 0;
	pb->inputs.start = 0;
	pb->weapon = 0;
}

struct bullet *add_bullet(warf_t *w){
	struct bullet *pb;
	pb = malloc(sizeof*pb);
	BulletInit(pb, w, &bullet_s);
	return pb;
}

struct bullet *BulletNew(warf_t *w, entity_t *owner, double damage){
	struct bullet *pb;
	pb = add_bullet(w);
	pb->owner = owner;
	pb->damage = damage;
	pb->mass = damage;
	pb->life = 0.;
	return pb;
}

static double noise_pixel(int x, int y, int bit){
	struct random_sequence rs;
	initfull_rseq(&rs, x + (bit << 16), y);
	return drseq(&rs);
}

double perlin_noise_pixel(int x, int y, int bit){
	int ret = 0, i;
	double sum = 0., maxv = 0., f = 1.;
	double persistence = 0.5;
	for(i = 3; 0 <= i; i--){
		int cell = 1 << i;
		double a00, a01, a10, a11, fx, fy;
		a00 = noise_pixel(x / cell, y / cell, bit);
		a01 = noise_pixel(x / cell, y / cell + 1, bit);
		a10 = noise_pixel(x / cell + 1, y / cell, bit);
		a11 = noise_pixel(x / cell + 1, y / cell + 1, bit);
		fx = (double)(x % cell) / cell;
		fy = (double)(y % cell) / cell;
		sum += ((a00 * (1. - fx) + a10 * fx) * (1. - fy)
			+ (a01 * (1. - fx) + a11 * fx) * fy) * f;
		maxv += f;
		f *= persistence;
	}
	return sum / maxv;
}

void explotex(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i, j;
	static GLuint texnames[8] = {0};
	struct random_sequence rs;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texnames[0]){
		GLubyte tex4[16*16][4];
		BITMAPINFO *bmi;
		int t;
		init_rseq(&rs, 437385);
		bmi = ReadBitmap("explode.bmp");
		glGenTextures(6, texnames);
		for(t = 0; t < 8; t++){
			int x, y;
			int cols = bmi->bmiHeader.biClrUsed ? bmi->bmiHeader.biClrUsed : 16;
			const unsigned char *head = (const unsigned char*)&bmi->bmiColors[cols];
			{
//				tex4 = malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
				for(y = 0; y < bmi->bmiHeader.biHeight; y++){
					unsigned char *buf = head + (bmi->bmiHeader.biWidth * bmi->bmiHeader.biBitCount + 31) / 32 * 4 * y;
					for(x = 0; x < 16; x++){
						int pos = x + y * 16, idx = 0xf & (buf[(x + t * 16) / 2] >> ((x + t * 16 + 1) % 2 * 4));
						tex4[pos][0] = bmi->bmiColors[idx].rgbRed;
						tex4[pos][1] = bmi->bmiColors[idx].rgbGreen;
						tex4[pos][2] = bmi->bmiColors[idx].rgbBlue;
						tex4[pos][3] = idx ? 128 + rseq(&rs) % 128 : 0;
					}
				}
			}
			glBindTexture(GL_TEXTURE_2D, texnames[t]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex4);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	}
	{
		int idx = 8 - 8 * pl->life / 1.;
		if(0 <= idx && idx < 8)
			glBindTexture(GL_TEXTURE_2D, texnames[idx]);
	}
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	initfull_rseq(&rs, 6365, (unsigned long)pl);
	glPushMatrix();
	gldScaled(pl->len);
	glMultMatrixd(dd->invrot);
	glColor4f(1, 1, 1, 1.);
	glBegin(GL_QUADS);
	glTexCoord2d(0., 0.); glVertex3d(-1., -1., 0.);
	glTexCoord2d(1., 0.); glVertex3d(+1., -1., 0.);
	glTexCoord2d(1., 1.); glVertex3d(+1., +1., 0.);
	glTexCoord2d(0., 1.); glVertex3d(-1., +1., 0.);
	glEnd();
	glPopMatrix();
	glPopMatrix();
	glPopAttrib();
}

static int explosmokecmp(const double **pa, const double **pb){
	return **pa < **pb ? -1 : **pa == **pb ? 0 : 1;
}

void explosmoke(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	int i, j;
	static GLuint texname = 0;
	struct random_sequence rs;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texname){
		GLubyte texbits[64][64][4];
		init_rseq(&rs, 437385);
		for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
			int a, r;
			a = rseq(&rs) % 64 == 0;
			r = 128 + perlin_noise_pixel(i, j, 0) * 64;
			texbits[i][j][0] = r;
			r = 64 + perlin_noise_pixel(i, j, 1) * 32;
			texbits[i][j][1] = r;
			r = 12 + perlin_noise_pixel(i, j, 2) * 12 - 8;
			texbits[i][j][2] = r;
			r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32 + a * 128;
			texbits[i][j][3] = MIN(255, MAX(0, r));
		}
		glGenTextures(1, &texname);
		glBindTexture(GL_TEXTURE_2D, texname);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, texbits);
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, texname);
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
	initfull_rseq(&rs, 6365, (unsigned long)pl);
	{
		double rad;
		GLfloat t;
		avec3_t dofs[4];
		double arad[4];
		double asp[4], *pasp[4];
		rad = pl->len * (1. - pl->life * .8);
		t = pl->life * pl->life;
		for(i = 3; 0 <= i; i--){
			for(j = 0; j < 3; j++)
				dofs[i][j] = (drseq(&rs) - .5) * rad;
			arad[i] = rad * (drseq(&rs) * .4 + .5);
			asp[i] = VECSP(dd->viewdir, dofs[i]);
			pasp[i] = &asp[i];
		}
		qsort(pasp, numof(pasp), sizeof *pasp, explosmokecmp);
		for(i = 3; 0 <= i; i--){
			glPushMatrix();
			gldTranslate3dv(dofs[pasp[i] - asp]);
			gldScaled(arad[pasp[i] - asp]);
			glMultMatrixd(dd->invrot);
			glColor4f(t, t * 2 - 1, t, t);
			glBegin(GL_QUADS);
			glTexCoord2d(0., 0.); glVertex3d(-1., -1., 0.);
			glTexCoord2d(1., 0.); glVertex3d(+1., -1., 0.);
			glTexCoord2d(1., 1.); glVertex3d(+1., +1., 0.);
			glTexCoord2d(0., 1.); glVertex3d(-1., +1., 0.);
			glEnd();
			glPopMatrix();
		}
	}
	glPopMatrix();
	glPopAttrib();
}

static void dirtsmoke(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv){
	double rad;
	struct random_sequence rs;
	int i, n = (int)pv;
	static GLuint texname[3] = {0};
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	if(!texname[n]){
		GLubyte texbits[64][64][4];
		int j;
		init_rseq(&rs, 3525);
		for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
			int a, r;
			a = rseq(&rs) % 64 == 0;
			if(n == 1){
				r = 191 + rseq(&rs) % 64;
				texbits[i][j][0] = r;
				texbits[i][j][1] = r;
				texbits[i][j][2] = r;
			}
			else{
				r = 32 + rseq(&rs) % 32 - 16;
				texbits[i][j][0] = r;
				r = 24 + rseq(&rs) % 24 - 12;
				texbits[i][j][1] = r;
				r = 12 + rseq(&rs) % 12 - 8;
				texbits[i][j][2] = r;
			}
			r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32 + a * 128;
			texbits[i][j][3] = MIN(255, MAX(0, r));
		}
		glGenTextures(1, &texname[n]);
		glBindTexture(GL_TEXTURE_2D, texname[n]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, texbits);
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, texname[n]);
/*	{
		avec3_t start, end;
		VECCPY(start, pl->pos);
		VECSADD(start, avec3_010, -rad);
		VECCPY(end, pl->pos);
		VECSADD(end, avec3_010, rad);
		gldTextureBeam(dd->viewpoint, start, end, rad * .25);
	}*/
	glPushMatrix();
	glTranslated(pl->pos[0], pl->pos[1], pl->pos[2]);
/*	glMultMatrixd(dd->invrot);*/
	{
		double phi;
		phi = -atan2(pl->pos[2] - dd->viewpoint[2], pl->pos[0] - dd->viewpoint[0]) + M_PI / 2.;
		glRotated(phi * deg_per_rad, 0, 1, 0);
	}
	rad = pl->len * (1. - pl->life * pl->life / .5 / .5);
	glScaled(rad * .25, rad, rad);
	glColor4ub(255,255,255, pl->life * pl->life / .5 / .5 * 255);
	glBegin(GL_QUADS);
	glTexCoord2d(0., 0.); glVertex3d(-1., -1., 0.);
	glTexCoord2d(1., 0.); glVertex3d(+1., -1., 0.);
	glTexCoord2d(1., 1.); glVertex3d(+1., +1., 0.);
	glTexCoord2d(0., 1.); glVertex3d(-1., +1., 0.);
	glEnd();
	glPopMatrix();
	glPopAttrib();
}


static void bulletkill(struct bullet *pb, warf_t *w, struct tent3d_line_list *tell, const double pyr[3], int hitground, const struct contact_info *ci){
	int j;
	if(!tell)
		return;
	if(1000. < pb->damage){
		struct tent3d_line_list *tell = w->tell;
		entity_t *pt;
		const double *const pos = pb->pos;
		pt = w->tl;
		for(; pt; pt = pt->next) if(pt->active && VECSDIST(pos, pt->pos) < pb->damage * pb->damage / 10000. / 10000.){
			((struct entity_private_static*)pt->vft)->takedamage(pt, pb->damage / (1. + VECSDIST(pos, pt->pos)), w, 0);
		}
		if(tell){
			int j;
			avec3_t gravity;
			w->vft->accel(w, &gravity, &pb->pos, &avec3_000);
			for(j = 0; j < 20; j++){
				double velo[3];
				velo[0] = .15 * (drseq(&w->rs) - .5);
				velo[1] = .15 * (drseq(&w->rs) - .5);
				velo[2] = .15 * (drseq(&w->rs) - .5);
				AddTeline3D(tell, pos, velo, .005, NULL, NULL, gravity,
					j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255), TEL3_HEADFORWARD | TEL3_FADEEND |  TEL3_REFLECT, 3. + 2. * drseq(&w->rs));
			}
			for(j = 0; j < 10; j++){
				double velo[3];
				velo[0] = .15 * (drseq(&w->rs) - .5);
				velo[1] = .15 * (drseq(&w->rs) - .5);
				velo[2] = .15 * (drseq(&w->rs) - .5);
				AddTefpol3D(w->tepl, pos, velo, gravity, &cs_fireburn,
					TEP3_REFLECT, pb->damage * .001 * (3. + 2. * drseq(&w->rs)));
			}

			{/* explode shockwave thingie */
				static const double pyr[3] = {M_PI / 2., 0., 0.};
				AddTeline3D(tell, pos, NULL, pb->damage * .0001, pyr, NULL, NULL, COLOR32RGBA(255,63,63,255), TEL3_EXPANDISK | TEL3_NOLINE, pb->damage * .001);
				if(ci && 0 <= ci->depth) /* no scorch in midair */
					AddTeline3D(tell, pos, NULL, pb->damage * .0001, pyr, NULL, NULL, COLOR32RGBA(0,0,0,255), TEL3_STATICDISK | TEL3_NOLINE, 3.);
			}
		}
	}
	if(pb->vft == &ExplosiveBullet_s || 30. < pb->damage){
		entity_t *pt;
		avec3_t pos;
		avec3_t accel;
		avec3_t gravity;
		w->vft->accel(w, &gravity, &pb->pos, &avec3_000);

		VECCPY(pos, pb->pos);
		if(ci)
			VECSADD(pos, ci->normal, ci->depth);

		for(pt = w->tl; pt; pt = pt->next) if(pt->active && VECSDIST(pos, pt->pos) < pb->damage * pb->damage / 20000. / 20000.){
			makedamage(pb, pt, w, (pb->damage * pb->damage / 20000. / 20000. - VECSDIST(pos, pt->pos)) * 20000. * 20000. / pb->damage, 0);
		}

		if(30. < pb->damage) for(j = 0; j < 10; j++){
			double velo[3];
			velo[0] = .05 * (drseq(&w->rs) - .5);
			velo[1] = .05 * (drseq(&w->rs) - .5);
			velo[2] = .05 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pos, velo, .001, NULL, NULL, gravity, COLOR32RGBA(255,127,0,255), TEL3_HEADFORWARD | TEL3_REFLECT, 1.5);
		}

		if(30. < pb->damage && ci){
			avec3_t dr, v;
			aquat_t q;
			amat4_t mat;
			double p;
			VECNORM(dr, ci->normal);

			/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
			q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

			VECVP(v, avec3_001, dr);
			p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
			VECSCALE(q, v, p);
			AddTeline3D(tell, pos, NULL, .01, q, NULL, NULL, COLOR32RGBA(255,127,63,255), TEL3_GLOW | TEL3_NOLINE | TEL3_NEAR | TEL3_QUAT, 1.);
		}

		if(0 < hitground && w->wmd){
			double pos[2] = {pb->pos[0], pb->pos[2]};
			AddWarmapDecal(w->wmd, pos, (void*)4);
		}
		AddTelineCallback3D(tell, pb->pos, NULL, pb->damage / 10000. + .003, NULL, NULL, NULL, explosmoke, NULL, 0, 1.);
	}
	else if(pb->vft != &ShotgunBullet_s || rseq(&w->rs) % 8 == 0/*VECSDIST(pb->pos, w->pl->pos) < .05*/){
#if 1
		avec3_t accel;
		int f;
		w->vft->accel(w, &accel, &pb->pos, &avec3_000);
/*		frexp(pb->damage, &f);*/
		if(0 <= hitground)
			AddTelineCallback3D(tell, pb->pos, NULL, pb->vft == &ShotgunBullet_s ? .01 : sqrt(pb->damage) * .5 * .01, NULL, NULL, accel, dirtsmoke, NULL, TEL3_INVROTATE, .5);
#elif 1
		AddTeline3D(tell, pb->pos, NULL, .002, NULL, NULL, NULL, COLOR32RGBA(191,127,0,255), TEL3_EXPANDGLOW | TEL3_NOLINE | TEL3_INVROTATE, .5);
#else
		for(j = 0; j < 10; j++){
			double velo[3];
			velo[0] = .001 * (drseq(&gsrs) - .5);
			velo[1] = .002 * (drseq(&gsrs));
			velo[2] = .001 * (drseq(&gsrs) - .5);
/*			AddTeline3D(tell, pb->pos, velo, pb->damage * .0002, NULL, NULL, gravity, COLOR32RGBA(191,127,0,255), TEL3_HEADFORWARD | TEL3_REFLECT, .5);*/
			AddTeline3D(tell, pb->pos, velo, .0001, NULL, NULL, gravity, COLOR32RGBA(191,127,0,255), TEL3_GLOW | TEL3_NOLINE | TEL3_INVROTATE, .5);
		}
#endif
		if(0 < hitground && w->wmd){
			double pos[2] = {pb->pos[0], pb->pos[2]};
			AddWarmapDecal(w->wmd, pos, (void*)1);
		}
	}
	if(30. < pb->damage){
		playWave3D(CvarGetString("sound_explosion"), pb->pos, w->pl->pos, w->pl->pyr, .6, .01, w->realtime);
	}
	else if(pb->vft != &ShotgunBullet_s || rseq(&w->rs) % 16 == 0/*VECSDIST(pb->pos, w->pl->pos) < .05*/){
		static int init = 0;
		static unsigned char wave[512];
		int ofs = 0;
		if(!init){
			int i;
			double freq[30];
			init = 1;
			for(i = 0; i < numof(freq); i++)
				freq[i] = (double)rseq(&w->rs) / UINT_MAX * 5. + .5;
			for(i = 0; i < numof(wave); i++){
				double rands = 0.;
				int j;
				for(j = 0; j < numof(freq); j++)
					rands += cos(((double)i * freq[j] / WAVEMIX_SAMPLERATE * pow(2., -(double)i * 10. / WAVEMIX_SAMPLERATE)) * 2 * M_PI * 500);
				wave[i] = (BYTE)(255 * (1. + (
		/*		+ (i < numof(wave) / 4 ? .5 * cos((double)i / SAMPLERATE * 2 * M_PI * 2200) : 0.)*/
				+ rands / 10.
	/*			+ cos((double)i / SAMPLERATE * 2 * M_PI * 330)
				+ 0.5 * cos(((double)i / SAMPLERATE * 2 * M_PI + .2) * 110)
				+ 0.5 * cos(((double)i / SAMPLERATE * 2 * M_PI + .8) * 550)*/)
				/ numof(wave) / 1 / 2. * (i < numof(wave) / 4 ? i * 4 : numof(wave) - (i - numof(wave) / 4))) / 2.);
			}
		}
		ofs = rseq(&w->rs) % (numof(wave) / 4);
		if(numof(wave) < ofs) ofs = 0;
		playMemoryWave3D(&wave[ofs], sizeof wave - ofs, 0, pb->pos, w->pl->pos, w->pl->pyr, .5, .01 * .01 * pb->damage, w->realtime);
	}
}

static int bullet_hit_callback(const struct otjEnumHitSphereParam *param, entity_t *pt){
	const avec3_t *src = param->src;
	const avec3_t *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	avec3_t *retpos = param->pos;
	avec3_t *retnorm = param->norm;
	void *hint = param->hint;
	struct entity_private_static *vft;
	struct bullet *pb = ((struct bullet**)hint)[1];
	warf_t *w = ((warf_t**)hint)[0];
	int *rethitpart = ((int**)hint)[2];
	avec3_t pos, nh;
	sufindex pi;
	double damage; /* calculated damage */
	int ret = 1;

	if(pb->owner == pt)
		return 0;
	vft = (struct entity_private_static*)pt->vft;
	if(!jHitSphere(pt->pos, vft->hitradius, pb->pos, pb->velo, dt))
		return 0;
	if(vft->tracehit){
		ret = vft->tracehit(pt, w, pb->pos, pb->velo, 0., dt, NULL, &pos, &nh);
		if(!ret)
			return 0;
		else if(rethitpart)
			*rethitpart = ret;
	}
	else{
		avec3_t sc;
		double t;
		sc[0] = sc[1] = sc[2] = vft->hitradius / 2.;
		if(!jHitBox(pt->pos, sc, pt->rot, pb->pos, pb->velo, 0., dt, &t, &pos, &nh)){
			return 0;
		}
	}

	if(vft->hitsuf){
		static const GLdouble rotaxis[16] = {
			0,0,-1,0,
			-1,0,0,0,
			0,1,0,0,
			0,0,0,1,
		};
		sufmodel_t mdl;
		amat4_t mat, mat2, rot;
		if(!(vft->altaxis & 2)){
			if(!vft->hitmdl.valid){
					amat4_t *pmat = vft->altaxis ? &mat : &vft->hitmdl.trans;
					MAT4IDENTITY(*pmat);
					MAT4SCALE(*pmat, vft->sufscale, vft->sufscale, vft->sufscale);
					if(vft->altaxis)
						mat4mp(vft->hitmdl.trans, *pmat, rotaxis);
				sufmodel_normalize(&vft->hitmdl);
			}
			mdl = vft->hitmdl;
/*				mdl.suf = vft->sufbase;
			MAT4IDENTITY(*pmat);*/
			MAT4IDENTITY(mat2);
			MAT4TRANSLATE(mat2, pt->pos[0], pt->pos[1], pt->pos[2]);
			if(vft->st.getrot)
				vft->st.getrot(pt, w, &rot);
			else
				pyrmat(pt->pyr, &rot);
			mat4mp(mat, mat2, rot);
			MAT4VP3(mdl.bs, mat, vft->hitmdl.bs);
			mat4mp(mdl.trans, mat, vft->hitmdl.trans);
/*				MAT4SCALE(*pmat, vft->sufscale, vft->sufscale, vft->sufscale);
			mdl.valid = 0;*/
			if(!jHitSufModel(&mdl, pb->pos, pb->velo, dt, NULL, &pos, &nh, &pi))
				return 0;
		}
		else{
			if(!jHitSufModel(&vft->hitmdl, pb->pos, pb->velo, dt, NULL, &pos, &nh, &pi))
				return 0;
		}
		VECCPY(pb->pos, pos);
		if(retpos)
			VECCPY(*retpos, pos);
		if(retnorm)
			VECCPY(*retnorm, nh);
	}
	else{
		VECSADD(pb->pos, pb->velo, dt);
		if(retpos)
			VECCPY(*retpos, pos);
		if(retnorm)
			VECCPY(*retnorm, nh);
	}
	return ret;
}
#endif
void Bullet::anim(double dt){
	if(!w)
		return;
	Bullet *const pb = this;
	struct contact_info ci;

	if(0 < pb->life && (pb->life -= dt) <= 0.){
/*		if(hitobj)
			*hitobj = NULL;
		return 0;*/
		w = NULL;
		return;
	}

	/* tank hit check */
	if(1){
//		struct otjEnumHitSphereParam param;
//		void *hint[3];
		Entity *pt;
/*		avec3_t pos, nh;
		int hitpart = 0;
		hint[0] = w;
		hint[1] = pb;
		hint[2] = &hitpart;
		param.root = w->otroot;
		param.src = pb->pos;
		param.dir = pb->velo;
		param.dt = dt;
		param.rad = 0.;
		param.pos = &pos;
		param.norm = &nh;
		param.flags = OTJ_CALLBACK;
		param.callback = bullet_hit_callback;
		param.hint = hint;*/
#if 1
//		pt = w->ot ? otjHitSphere(&w->ot[w->oti-1], pb->pos, pb->velo, dt, 0., NULL) : NULL;
		for(pt = w->el; pt; pt = pt->next){
#else
		for(pt = w->ot ? (otjEnumHitSphere(&param)) : w->tl; pt; pt = w->ot ? NULL : pt->next){
#endif
			sufindex pi;
			double damage; /* calculated damaga*/

/*			if(!w->ot && !bullet_hit_callback(&param, pt))
				continue;*/

			if(pt == pb || pt->w != w)
				continue;

			// if the ultimate owner of the objects is common, do not hurt yourself.
			Entity *bulletAncestor = pb;
			while(bulletAncestor->getOwner())
				bulletAncestor = bulletAncestor->getOwner();
			Entity *hitobjAncestor = pt;
			while(hitobjAncestor->getOwner())
				hitobjAncestor = hitobjAncestor->getOwner();
			if(bulletAncestor == hitobjAncestor)
				continue;

//			if(!jHitSphere(pt->pos, pt->hitradius(), pb->pos, pb->velo, dt))
//				continue;
			Vec3d nh, pos;
			int hitpart = pt->tracehit(pb->pos, pb->velo, 0., dt, NULL, &pos, &nh);
			if(hitpart == 0)
				continue;
#if 0
			}
			else{
				avec3_t sc;
				double t;
				sc[0] = sc[1] = sc[2] = vft->hitradius / 2.;
				if(!jHitBox(pt->pos, sc, pt->rot, pb->pos, pb->velo, 0., dt, &t, &pos, &nh)){
					continue;
				}
			}

			if(vft->hitsuf){
				static const GLdouble rotaxis[16] = {
					0,0,-1,0,
					-1,0,0,0,
					0,1,0,0,
					0,0,0,1,
				};
				sufmodel_t mdl;
				amat4_t mat, mat2, rot;
				if(!(vft->altaxis & 2)){
					if(!vft->hitmdl.valid){
							amat4_t *pmat = vft->altaxis ? &mat : &vft->hitmdl.trans;
							MAT4IDENTITY(*pmat);
							MAT4SCALE(*pmat, vft->sufscale, vft->sufscale, vft->sufscale);
							if(vft->altaxis)
								mat4mp(vft->hitmdl.trans, *pmat, rotaxis);
						sufmodel_normalize(&vft->hitmdl);
					}
					mdl = vft->hitmdl;
	/*				mdl.suf = vft->sufbase;
					MAT4IDENTITY(*pmat);*/
					MAT4IDENTITY(mat2);
					MAT4TRANSLATE(mat2, pt->pos[0], pt->pos[1], pt->pos[2]);
					if(vft->st.getrot)
						vft->st.getrot(pt, w, &rot);
					else
						pyrmat(pt->pyr, &rot);
					mat4mp(mat, mat2, rot);
					MAT4VP3(mdl.bs, mat, vft->hitmdl.bs);
					mat4mp(mdl.trans, mat, vft->hitmdl.trans);
	/*				MAT4SCALE(*pmat, vft->sufscale, vft->sufscale, vft->sufscale);
					mdl.valid = 0;*/
					if(!jHitSufModel(&mdl, pb->pos, pb->velo, dt, NULL, &pos, &nh, &pi))
						continue;
				}
				else{
					if(!jHitSufModel(&vft->hitmdl, pb->pos, pb->velo, dt, NULL, &pos, &nh, &pi))
						continue;
				}
				VECCPY(pb->pos, pos);
			}
			else{
				VECSADD(pb->pos, pb->velo, dt);
			}
#endif

			VECCPY(pb->pos, pos);

			pb->pos = pos;
			pt->bullethit(pb);
#if 0
			{ /* ricochet */
				if(w->tell && rseq(&w->rs) % (w->effects + 1) == 0){
					int j, n;
					Vec3d pyr, bvelo;
					bvelo = pb->velo - pt->velo;
					if(!vft->hitsuf){
#if 1
						aquat_t q;
						quatdirection(q, nh);
						AddTeline3D(tell, pos, pt->velo, pb->damage * .001 + .001, q, NULL, NULL, COLOR32RGBA(255,215,127,255), TEL3_NOLINE | TEL3_CYLINDER | TEL3_QUAT, .5 + .001 * pb->damage);
#else
						VECSUB(pos, pb->pos, pt->pos);
						pyr[0] = atan2(bvelo[1], sqrt(bvelo[0] * bvelo[0] + bvelo[2] * bvelo[2]));
						pyr[1] = atan2(bvelo[0], -bvelo[2]);
						pyr[2] = drseq(&w->rs) * M_PI;
						VECNORMIN(pos);
						VECSCALEIN(pos, ((struct entity_private_static*)pt->vft)->hitradius);
						VECADDIN(pos, pt->pos);
						AddTeline3D(tell, pos, pt->velo, pb->damage * .0001 + .001, quat, NULL, NULL, COLOR32RGBA(255,215,127,255), TEL3_NOLINE | TEL3_CYLINDER, .5 + .001 * pb->damage);
#endif
					}
					else{
						avec3_t delta;
						VECCPY(delta, nh);
						pyr[0] = atan2(delta[1], sqrt(delta[0] * delta[0] + delta[2] * delta[2]));
						pyr[1] = fmod(atan2(delta[0], -delta[2]) + 2 * M_PI, 2 * M_PI);
						pyr[2] = 0.;
						if(vft->bullethole){
							avec3_t da;
							VECSUB(delta, pos, pt->pos);
							vft->bullethole(pt, pi, pb->damage * .00001, delta, pyr);
						}
						AddTeline3D(tell, pos, pt->velo, pb->damage * .0001 + .001, pyr, NULL, NULL, COLOR32RGBA(255,215,127,255), TEL3_NOLINE | TEL3_CYLINDER, .5 + .001 * pb->damage);
					}
					w->effects++;
					if(tell){
						int j, n;
						frexp(pb->damage, &n);
						for(j = 0; j < n; j++){
							double velo[3];
							velo[0] = .15 * (drseq(&w->rs) - .5);
							velo[1] = .15 * (drseq(&w->rs) - .5);
							velo[2] = .15 * (drseq(&w->rs) - .5);
							AddTeline3D(tell, pos, velo, .001, NULL, NULL, w->gravity,
								j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255),
								TEL3_HEADFORWARD | TEL3_FADEEND, .5 + drseq(&w->rs) * .5);
						}
					}
#if 0
					{
						frexp(pb->damage, &n);
						VECNORMIN(bvelo);
						VECSCALEIN(bvelo, -(.05 + .001 * pb->damage));
						for(j = 0; j < n; j++){
							double velo[3], len[3];
							GLubyte r, g, b;
							r = 191 + rseq(&w->rs) % 64;
							g = 128 + rseq(&w->rs) % 128;
							b = 128 + rseq(&w->rs) % 64;
							velo[0] = bvelo[0] * drseq(&gsrs);
							velo[1] = bvelo[1] * drseq(&gsrs);
							velo[2] = bvelo[2] * drseq(&gsrs);
							VECNORM(len, velo);
	/*						pyr[0] = atan2(velo[0], -velo[2]) + M_PI;
							pyr[1] = atan2(velo[1], sqrt(velo[0] * velo[0] + velo[2] * velo[2]));
							pyr[2] = 0.;*/
							VECADDIN(velo, pt->velo);
							AddTeline3D(tell, pos, velo, pb->damage * .0001 + .001, len, NULL, gravity,
								COLOR32RGBA(r,g,b,255),
								TEL3_THICK | TEL3_FADEEND, .15 + .15 * drseq(&gsrs));
						}
					}
#endif
				}
			}
			damage = pb->damage;

			{
			extern double g_bulletimpact;
			extern struct entity_private_static container_s;
			if(/*0 && pt->vft == &tank_s ||*/ pt->vft == &container_s || ((struct entity_private_static*)pt->vft)->altaxis & 4){
				double lmomentum;
				avec3_t dr, momentum, amomentum;
				VECSUB(dr, pb->pos, pt->pos);
				VECSUBIN(dr, ((struct entity_private_static*)pt->vft)->cog);
				lmomentum = g_bulletimpact * pb->mass/* * VECLEN(pb->velo)*/;
				VECSCALE(momentum, pb->velo, lmomentum);
				RigidAddMomentum(pt, dr, momentum);
#if 0
				VECSADD(pt->velo, momentum, 1. / pt->mass);

				/* vector product of dr and momentum becomes angular momentum */
				VECVP(amomentum, dr, momentum);
				VECSADD(pt->omg, amomentum, 1. / pt->moi);
#endif
			}
			}

			if(hitobj)
				*hitobj = pt;
#endif

			pt->takedamage(this->damage, hitpart);
//			makedamage(pb, pt, w, pb->damage, hitpart);

//			bulletkill(pb, w, w->tell, avec3_000, -1, NULL);
			w = NULL;

/*			{
				avec3_t delta;
				VECSUB(delta, pb->pos, pt->pos);
				VECNORM(ci.normal, delta);
				ci.depth = 0.;
				VECCPY(ci.velo, pt->velo);
				bulletkill(pb, w, w->tell, avec3_000, 1, &ci);
			}*/

			return;
		}while(0);
	}

#if 0 // world hit
	{
		int i;
		for(i = 0; i <= 4; i++){
			avec3_t pos;
			VECCPY(pos, pb->pos);
			VECSADD(pos, pb->velo, dt * i / 4);
			if(w->vft->pointhit(w, &pos, &pb->velo, dt, &ci) /*pb->pos[1] <= (w->map ? warmapheight(w->map, pb->pos[0], pb->pos[2], NULL) : 0.)*/ /*|| BULLETRANGE * BULLETRANGE < pb->pos[0] * pb->pos[0] + pb->pos[1] * pb->pos[1] + pb->pos[2] * pb->pos[2]*/){
				static const double pyr[3] = {M_PI / 2., 0., 0.};
				VECCPY(pb->pos, pos);
				bulletkill(pb, w, w->tell, pyr, 1, &ci);
				if(hitobj)
					*hitobj = NULL;
				return 0;
			}
		}
	}
#endif

	if(pb->grav){
		Vec3d accel = w->accel(pb->pos, pb->velo);
		pb->velo += accel * dt;
	}
	pb->pos += pb->velo * dt;
	return;
}

double Bullet::hitradius(){
	return 0.;
}

Entity *Bullet::getOwner(){
	return owner;
}

void Bullet::postframe(){
	if(!w)
		return;
	if(owner && owner->w != w)
		owner = NULL;
}

void Bullet::drawtra(wardraw_t *wd){
	double length = (this->damage * .25 + VECSLEN(this->velo) * 5 * 5 + 20) * .0005;
	double span, scale, f;
	double pixels;
	double width = this->damage * .00005, wpix;

	f = 2. * (this->velo).len() * length;
	span = MIN(f, .1);
	length *= span / f;

//	if(glcullFrustum(pb->pos, span, wd->pgc))
//		return;
	scale = fabs(wd->vw->gc->scale(pos));
	pixels = span * scale;
	if(pixels < 2)
		return;

	wpix = width * scale / 2.;

	/*	glLineWidth((GLfloat)m / DISTANCE(view, pb->pos));*/
	if(/*VECSDIST(pb->pos, wd->view) < .2 * .2*/ 1 < wpix){
		Vec3d start, end;
		glColor4ub(255,127,0,255);
		start = this->pos;
		start += this->velo * length;
		end = this->pos;
		end += this->velo * -length;
		if(damage < 100.){
			static GLuint texname = 0;
			static const GLfloat envcolor[4] = {.5,0,0,1};
			glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
			if(!texname){
				GLubyte texbits[64][64][2];
				int i, j;
				for(i = 0; i < 64; i++) for(j = 0; j < 64; j++){
					int a, r;
					r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
					texbits[i][j][0] = 255 - MIN(255, MAX(0, r));
					r = (32 * 32 - (i - 32) * (i - 32) - (j - 32) * (j - 32)) * 255 / 32 / 32;
					texbits[i][j][1] = MIN(255, MAX(0, r));
				}
				glGenTextures(1, &texname);
				glBindTexture(GL_TEXTURE_2D, texname);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 64, 64, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, texbits);
			}
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glColor4f(1,.5,0,1);
			glBindTexture(GL_TEXTURE_2D, texname);
			gldTextureBeam(wd->vw->pos, start, end, width);
			glPopAttrib();
/*			gldGradBeam(wd->view, start, end, width, COLOR32RGBA(127,0,0,0));*/
		}
		else
			gldBeam(wd->vw->pos, start, end, length / 100.);
	}
	else if(.05 < wpix){
		glBegin(GL_LINES);
		glColor4d(1., .5, 0., wpix);
		glVertex3dv(pos + velo * length);
/*		if(pb->damage < 50.)
			glColor4ub(127, 0, 0, 0);*/
		glVertex3dv(pos - velo * length);
		glEnd();
	}
}

bool Bullet::isTargettable()const{
	return false;
}
