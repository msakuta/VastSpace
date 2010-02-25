#include "bullet.h"
#include "coordsys.h"
#include "astro.h"
#include "player.h"
#include "judge.h"
#include "serial_util.h"
//#include "warutil.h"
extern "C"{
#include "bitmap.h"
#include "material.h"
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

const unsigned Bullet::classid = registerClass("Bullet", Conster<Bullet>);

void Bullet::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << damage;
	sc.o << life;
	sc.o << runlength;
	sc.o << owner;
	sc.o << grav;
}

void Bullet::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> damage;
	sc.i >> life;
	sc.i >> runlength;
	sc.i >> owner;
	sc.i >> grav;
}

void sufmodel_normalize(sufmodel_t *mdl);

/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_fireburn[] = {
	{0.03, COLOR32RGBA(255, 255, 255, 255)},
	{0.035, COLOR32RGBA(255, 255, 128, 255)},
	{0.035, COLOR32RGBA(128, 32, 32, 127)},
};
static const struct color_sequence cs_fireburn = DEFINE_COLSEQ(cnl_fireburn, (COLORREF)-1, 0.1);


#if 0

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
#endif

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
			unsigned char *head = (unsigned char*)&bmi->bmiColors[cols];
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

static int explosmokecmp(const void *pa, const void *pb){
	return **(const double**)pa < **(const double**)pb ? -1 : **(const double**)pa == **(const double**)pb ? 0 : 1;
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

#if 0
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
#endif

#define SQRT2P2 (M_SQRT2/2.)

void Bullet::bulletkill(int hitground, const struct contact_info *ci){
	int j;
	struct tent3d_line_list *tell = w->getTeline3d();
	struct tent3d_fpol_list *tepl = w->getTefpol3d();
	if(!tell)
		return;
	if(1000. < this->damage){
		Entity *pt;
		pt = w->el;
		for(; pt; pt = pt->next) if(pt->w == w && (pos - pt->pos).slen() < damage * damage / 10000. / 10000.){
			pt->takedamage(damage / (1. + VECSDIST(pos, pt->pos)), 0);
		}
		if(tell){
			int j;
			Vec3d gravity =	w->accel(pos, vec3_000);
			for(j = 0; j < 20; j++){
				double velo[3];
				velo[0] = .15 * (drseq(&w->rs) - .5);
				velo[1] = .15 * (drseq(&w->rs) - .5);
				velo[2] = .15 * (drseq(&w->rs) - .5);
				AddTeline3D(tell, pos, velo, .005, quat_u, vec3_000, gravity,
					j % 2 ? COLOR32RGBA(255,255,255,255) : COLOR32RGBA(255,191,63,255), TEL3_HEADFORWARD | TEL3_FADEEND |  TEL3_REFLECT, 3. + 2. * drseq(&w->rs));
			}
			for(j = 0; j < 10; j++){
				double velo[3];
				velo[0] = .15 * (drseq(&w->rs) - .5);
				velo[1] = .15 * (drseq(&w->rs) - .5);
				velo[2] = .15 * (drseq(&w->rs) - .5);
				AddTefpol3D(tepl, pos, velo, gravity, &cs_fireburn,
					TEP3_REFLECT, damage * .001 * (3. + 2. * drseq(&w->rs)));
			}

			{/* explode shockwave thingie */
				static const Quatd pyr(SQRT2P2, 0., 0., SQRT2P2);
				AddTeline3D(tell, pos, vec3_000, damage * .0001, pyr, vec3_000, vec3_000, COLOR32RGBA(255,63,63,255), TEL3_EXPANDISK | TEL3_NOLINE, damage * .001);
				if(ci && 0 <= ci->depth) /* no scorch in midair */
					AddTeline3D(tell, pos, vec3_000, damage * .0001, pyr, vec3_000, vec3_000, COLOR32RGBA(0,0,0,255), TEL3_STATICDISK | TEL3_NOLINE, 3.);
			}
		}
	}
	if(/*pb->vft == &ExplosiveBullet_s || */30. < damage){
		Entity *pt;
		Vec3d pos;
		Vec3d accel;
		Vec3d gravity = w->accel(this->pos, avec3_000);

		pos = this->pos;
		if(ci)
			pos += ci->normal * ci->depth;

/*		for(pt = w->el; pt; pt = pt->next) if(pt->w != w && (pos - pt->pos).slen() < damage * damage / 20000. / 20000.){
			makedamage(pb, pt, w, (pb->damage * pb->damage / 20000. / 20000. - VECSDIST(pos, pt->pos)) * 20000. * 20000. / pb->damage, 0);
		}*/

		if(30. < damage) for(j = 0; j < 5; j++){
			double velo[3];
			velo[0] = .05 * (drseq(&w->rs) - .5);
			velo[1] = .05 * (drseq(&w->rs) - .5);
			velo[2] = .05 * (drseq(&w->rs) - .5);
			AddTeline3D(tell, pos, velo, .001, quat_u, vec3_000, gravity, COLOR32RGBA(255,127,0,255), TEL3_HEADFORWARD | TEL3_REFLECT, 1.5);
		}

		if(30. < damage && ci){
			Vec3d dr, v;
			Quatd q;
			Mat4d mat;
			double p;
			dr = ci->normal.norm();

			/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
			q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

			v = vec3_001.vp(dr);
			p = sqrt(1. - q[3] * q[3]) / v.len();
			q = v * p;
			AddTeline3D(tell, pos, vec3_000, .01, q, vec3_000, vec3_000, COLOR32RGBA(255,127,63,255), TEL3_GLOW | TEL3_NOLINE | TEL3_NEAR | TEL3_QUAT, 1.);
		}

/*		if(0 < hitground && w->wmd){
			double pos[2] = {pb->pos[0], pb->pos[2]};
			AddWarmapDecal(w->wmd, pos, (void*)4);
		}*/
		AddTelineCallback3D(tell, this->pos, vec3_000, damage / 10000. + .003, quat_u, vec3_000, vec3_000, explosmoke, NULL, 0, 1.);
	}
#if 0
	else if(pb->vft != &ShotgunBullet_s || rseq(&w->rs) % 8 == 0/*VECSDIST(pb->pos, w->pl->pos) < .05*/){
#if 0
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
		if(0 < hitground && w->wmd){
			double pos[2] = {pb->pos[0], pb->pos[2]};
			AddWarmapDecal(w->wmd, pos, (void*)1);
		}
#endif
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
#endif
}

static int bullet_hit_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	const Vec3d *src = param->src;
	const Vec3d *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	Vec3d *retpos = param->pos;
	Vec3d *retnorm = param->norm;
	void *hint = param->hint;
//	struct entity_private_static *vft;
	Bullet *pb = ((Bullet**)hint)[1];
	WarField *w = pb->w;
	int *rethitpart = ((int**)hint)[2];
	Vec3d pos, nh;
	sufindex pi;
	double damage; /* calculated damage */
	int ret = 1;

	// if the ultimate owner of the objects is common, do not hurt yourself.
	Entity *bulletAncestor = pb->getUltimateOwner();
	Entity *hitobjAncestor = pt->getUltimateOwner();
	if(bulletAncestor == hitobjAncestor)
		return 0;

//	vft = (struct entity_private_static*)pt->vft;
	if(!jHitSphere(pt->pos, pt->hitradius() + pb->hitradius(), pb->pos, pb->velo, dt))
		return 0;
	{
		ret = pt->tracehit(pb->pos, pb->velo, pb->hitradius(), dt, NULL, &pos, &nh);
		if(!ret)
			return 0;
		else if(rethitpart)
			*rethitpart = ret;
	}
/*	else{
		Vec3d sc;
		double t;
		sc[0] = sc[1] = sc[2] = pt->hitradius() / 2.;
		if(!jHitBox(pt->pos, sc, pt->rot, pb->pos, pb->velo, 0., dt, &t, &pos, &nh)){
			return 0;
		}
	}*/

	{
		pb->pos += pb->velo * dt;
		if(retpos)
			*retpos = pos;
		if(retnorm)
			*retnorm = nh;
	}
	return ret;
}

void Bullet::anim(double dt){
	WarSpace *ws = *w;
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
		struct otjEnumHitSphereParam param;
		void *hint[3];
		Entity *pt;
		Vec3d pos, nh;
		int hitpart = 0;
		hint[0] = w;
		hint[1] = pb;
		hint[2] = &hitpart;
		param.root = ws->otroot;
		param.src = &pb->pos;
		param.dir = &pb->velo;
		param.dt = dt;
		param.rad = 0.;
		param.pos = &pos;
		param.norm = &nh;
		param.flags = OTJ_CALLBACK;
		param.callback = bullet_hit_callback;
		param.hint = hint;
#if 0
//		pt = w->ot ? otjHitSphere(&w->ot[w->oti-1], pb->pos, pb->velo, dt, 0., NULL) : NULL;
		for(pt = w->el; pt; pt = pt->next){
#else
		for(pt = ws->ot ? (otjEnumHitSphere(&param)) : ws->el; pt; pt = ws->ot ? NULL : pt->next){
#endif
			sufindex pi;
			double damage; /* calculated damaga*/

			if(!ws->ot && !bullet_hit_callback(&param, pt))
				continue;

			pb->pos = pos;
			pt->bullethit(pb);
#if 1
			{ /* ricochet */
				if(ws->tell && rseq(&w->rs) % (ws->effects + 1) == 0){
/*					int j, n;
					Vec3d pyr, bvelo;
					bvelo = pb->velo - pt->velo;
					Vec3d delta = pos - pt->pos;
					pt->bullethole(pi, pb->damage * .00001, delta, Quatd::direction(delta));
					AddTeline3D(w->tell, pos, pt->velo, pb->damage * .0001 + .001, pyr, NULL, NULL, COLOR32RGBA(255,215,127,255), TEL3_NOLINE | TEL3_CYLINDER, .5 + .001 * pb->damage);*/
				}
				ws->effects++;
				if(ws->tell){
					Vec3d accel = w->accel(pb->pos, pb->velo);
					int j, n;
					frexp(pb->damage, &n);
					n /= 4;
					for(j = 0; j < n; j++){
						double velo[3];
						velo[0] = .15 * (drseq(&w->rs) - .5);
						velo[1] = .15 * (drseq(&w->rs) - .5);
						velo[2] = .15 * (drseq(&w->rs) - .5);
						AddTeline3D(ws->tell, pos, velo, .001, quat_u, vec3_000, accel,
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
			damage = pb->damage;

#if 0
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
#endif

			if(pt->w == w) if(!pt->takedamage(this->damage, hitpart)){
				extern int bullet_kills, missile_kills;
				if(!strcmp("Bullet", classname()))
					bullet_kills++;
				else if(!strcmp("Missile", classname()))
					missile_kills++;
			}
//			makedamage(pb, pt, w, pb->damage, hitpart);

			bulletkill(-1, NULL);
			w = NULL;

			extern int bullet_hits;
			bullet_hits++;

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
	Vec3d move = pb->velo * dt;
	runlength += move.len();
	pb->pos += move;
}

double Bullet::hitradius()const{
	return 0.;
}

Entity *Bullet::getOwner(){
	return owner;
}

void Bullet::postframe(){
	st::postframe();
	if(!w)
		return;
	if(owner && owner->w != w)
		owner = NULL;
}

void Bullet::drawtra(wardraw_t *wd){
	double length = (this->damage * .25 + VECSLEN(this->velo) * 5 * 5 + 20) * .0005;
	double span, scale;
	double pixels;
	double width = this->damage * .00005, wpix;

	Vec3d velo = this->velo - wd->vw->velo;
	double velolen = velo.len();
	double f = 1. * velolen * length;
	span = MIN(f, .1);
	length *= span / f;

	if(wd->vw->gc->cullFrustum(pos, span))
		return;
	scale = fabs(wd->vw->gc->scale(pos));
	pixels = span * scale;
	if(pixels < 2)
		return;

	wpix = width * scale;

	/*	glLineWidth((GLfloat)m / DISTANCE(view, pb->pos));*/
	if(/*VECSDIST(pb->pos, wd->view) < .2 * .2*/ 1 < wpix){
		Vec3d start, end;
		glColor4ub(255,127,0,255);
		start = this->pos;
		end = this->pos;
		end += velo * -(runlength / velolen < length ? runlength / velolen : length);
		if(true || damage < 500.){
			static GLuint texname = 0;
			static const GLfloat envcolor[4] = {.5,0,0,1};
			glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
#if 1
			if(!texname){
				suftexparam_t stp;
				stp.flags = STP_ENV | STP_MAGFIL | STP_MINFIL;
				stp.env = GL_REPLACE;
				stp.magfil = GL_LINEAR;
				stp.minfil = GL_LINEAR;
				texname = CallCacheBitmap5("bullet.bmp", "bullet.bmp", &stp, NULL, NULL);
			}
			glCallList(texname);
/*			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glRotatef(90, 0, 0, 1);
			glScalef(9./32., 1, 1);
			glMatrixMode(GL_MODELVIEW);*/
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE); // Add blend
			gldTextureBeam(wd->vw->pos, start, end, length / width < 10. ? length / 10. : width);
/*			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);*/
#else
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
#endif
			glPopAttrib();
		}
		else
			gldBeam(wd->vw->pos, start, end, length / 100.);
	}
	else if(.05 < wpix){
		glBegin(GL_LINES);
		glColor4d(1., .75, .25, wpix);
		glVertex3dv(pos);
		glVertex3dv(pos - velo * (runlength / velolen < length ? runlength / velolen : length));
		glEnd();
	}
}

bool Bullet::isTargettable()const{
	return false;
}
