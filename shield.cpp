#include "shield.h"
#include "Viewer.h"
#include "Bullet.h"
#include "draw/WarDraw.h"
#include <cpplib/quat.h>
extern "C"{
#include <clib/gl/gldraw.h>
}


class ShieldEffect::Wavelet{
public:
	Quatd rot;
	double phase;
	double amount;
	Wavelet *next;
	void draw(const Entity *pt, double rad)const;
};

static class ShieldEffect::WaveletList{
public:
	int cactv; /* count of active elements for debugging, actually unnecessary */
	Wavelet *lfree;
	Wavelet list[1];
	static void free(Wavelet **);
	static void draws(const Entity *pt, const Wavelet *sw, double rad);
} *swlist;


ShieldEffect::~ShieldEffect(){
	// Free all allocated wavelets before dying
	Wavelet **pswlactv = &p;
	Wavelet **psw;
	for(psw = pswlactv; *psw;){
		Wavelet *sw = *psw;
		WaveletList::free(psw);
	}
}

void drawShieldSphere(const double pos[3], const avec3_t viewpos, double radius, const GLubyte color[4], const double irot[16]);


#define SHIELDW_MAX 0x100

void ShieldEffect::bullethit(const Entity *pe, const Bullet *pb, double maxshield){
	Quatd rot;
	{
		Vec3d pos, velo;
		double pi;
		Vec3d dr, v;

		dr = pb->pos - pe->pos;
		dr.normin();

		/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
		rot[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

		v = vec3_001.vp(dr);
		pi = sqrt(1. - rot[3] * rot[3]) / VECLEN(v);
		(Vec3d&)rot = v * pi;
	}
	double rdamage = pb->damage / maxshield;
	double amount = MIN(rdamage * 100. + .2, 5.);

	Wavelet **plactv = &p;
	if(!swlist){
		swlist = (WaveletList*)malloc(offsetof(WaveletList, list) + SHIELDW_MAX * sizeof *swlist->list);
		if(swlist){
			int i;
			for(i = 0; i < SHIELDW_MAX; i++){
				Wavelet *p = &swlist->list[i];
				p->amount = 0.;
				p->next = i < SHIELDW_MAX - 1 ? p+1 : NULL;
			}
			swlist->cactv = 0;
			swlist->lfree = swlist->list;
		}
	}
	if(!swlist || !swlist->lfree)
		return;
	{
		Wavelet *p = swlist->lfree;
		swlist->lfree = p->next;
		p->next = *plactv;
		*plactv = p;
		p->rot = rot;
		p->phase = 0.;
		p->amount = amount;
		swlist->cactv++;
		return;
	}
}

void ShieldEffect::takedamage(double rdamage){
	acty = MIN(acty + rdamage * 100., 5.);
}

void ShieldEffect::WaveletList::free(Wavelet **pp){
	Wavelet *p = *pp;
	*pp = p->next;
	p->next = swlist->lfree;
	swlist->lfree = p;
	swlist->cactv--;
}

static double noise_pixel_1d(int x, int bit){
	struct random_sequence rs;
	init_rseq(&rs, x);
	return drseq(&rs);
}

double perlin_noise_1d(int x, int bit){
	int ret = 0, i;
	double sum = 0., maxv = 0., f = 1.;
	double persistence = 0.5;
	for(i = 3; 0 <= i; i--){
		int cell = 1 << i;
		double a0, a1, fx;
		a0 = noise_pixel_1d(x / cell, bit);
		a1 = noise_pixel_1d(x / cell + 1, bit);
		fx = (double)(x % cell) / cell;
		sum += (a0 * (1. - fx) + a1 * fx) * f;
		maxv += f;
		f *= persistence;
	}
	return sum / maxv;
}

void ShieldEffect::Wavelet::draw(const Entity *pt, double rad)const{
	const Wavelet *const sw = this;
	const double (*cuts)[2];
	double ampl;
	double s0, c0, s1, c1, s2, c2;
	int i, inten, randbase;
	if(sw->amount == 0.)
		return;
	{
		struct random_sequence rs;
		init_rseq(&rs, (unsigned long)sw);
		randbase = rseq(&rs);
	}
/*	s0 = sin(sw->phase - .02 * M_PI);
	c0 = cos(sw->phase - .02 * M_PI);
	s1 = sin(sw->phase);
	c1 = cos(sw->phase);
	s2 = sin(sw->phase + .02 * M_PI);
	c2 = cos(sw->phase + .02 * M_PI);*/
	cuts = CircleCuts(32);
	glPushMatrix();
	gldTranslate3dv(pt->pos);
	gldMultQuat(sw->rot);
	gldScaled(rad);
	ampl = sin(sw->phase);
	inten = int(128 * (1. - 1. / (1. + sw->amount))/*sw->amount < .3 ? sw->amount / .3 * 255 : 255*/);
	glBegin(GL_QUAD_STRIP);
	for(i = 0; i <= 32; i++){
		int i1 = i % 32;
		double phase = sw->phase;
		phase += ampl * (perlin_noise_1d(i1 + randbase, 3) - .5) * M_PI / 4.;
		s0 = sin(phase - .02 * M_PI);
		c0 = cos(phase - .02 * M_PI);
		s1 = sin(phase);
		c1 = cos(phase);
		s2 = sin(phase + .02 * M_PI);
		c2 = cos(phase + .02 * M_PI);
		glColor4ub(255,0,255, 0);
		glVertex3d(s0 * cuts[i1][0], s0 * cuts[i1][1], c0);
		glColor4ub(0,255,255, inten);
		glVertex3d(s1 * cuts[i1][0], s1 * cuts[i1][1], c1);
	}
	glEnd();
	glBegin(GL_QUAD_STRIP);
	for(i = 0; i <= 32; i++){
		int i1 = i % 32;
		double phase = sw->phase;
		phase += ampl * (perlin_noise_1d(i1 + randbase, 3) - .5) * M_PI / 4.;
		s0 = sin(phase - .02 * M_PI);
		c0 = cos(phase - .02 * M_PI);
		s1 = sin(phase);
		c1 = cos(phase);
		s2 = sin(phase + .02 * M_PI);
		c2 = cos(phase + .02 * M_PI);
		glColor4ub(0,255,255, inten);
		glVertex3d(s1 * cuts[i1][0], s1 * cuts[i1][1], c1);
		glColor4ub(0,127,255, 0);
		glVertex3d(s2 * cuts[i1][0], s2 * cuts[i1][1], c2);
	}
	glEnd();
	glPopMatrix();
}

void ShieldEffect::draw(wardraw_t *wd, const Entity *pt, double rad, double ratio){
	if(0. < ratio && 0. < acty){
		GLubyte col[4] = {0,127,255,255};
		Vec3d dr;
		Mat4d irot;
		col[0] = 128 * (1. - ratio);
		col[2] = 255 * (ratio);
		col[3] = 128 * (1. - 1. / (1. + acty));
		dr = wd->vw->pos - pt->pos;
		gldLookatMatrix(~irot, ~dr);
		drawShieldSphere(pt->pos, wd->vw->pos, rad, col, irot);
	}
	for(Wavelet *sw = p; sw; sw = sw->next)
		sw->draw(pt, rad);
}

void ShieldEffect::anim(double dt){

	if(acty < dt)
		acty = 0.;
	else
		acty -= dt;

	Wavelet **pswlactv = &p;
	Wavelet **psw;
	for(psw = pswlactv; *psw;){
		Wavelet *sw = *psw;
		if(sw->amount < dt){
			sw->amount = 0.;
			WaveletList::free(psw);
		}
		else{
			sw->phase += sw->amount * dt;
			sw->amount -= dt;
			psw = &(*psw)->next;
		}
	}
}



#define SQRT2P2 (M_SQRT2/2.)

void drawShieldSphere(const double pos[3], const avec3_t viewpos, double radius, const GLubyte color[4], const double irot[16]){
	int i, j;
	const double (*cuts)[2];
	double jcuts0[4][2];
	double (*jcuts)[2] = jcuts0;
	double tangent;
	GLubyte colors[4][4];

	tangent = acos(radius / VECDIST(pos, viewpos));

	colors[3][0] = color[0];
	colors[3][1] = color[1];
	colors[3][2] = color[2];
	colors[3][3] = color[3];
	colors[0][0] = 255;
	colors[0][1] = 0;
	colors[0][2] = 128;
	colors[0][3] = 0;

	for(j = 1; j <= 2; j++) for(i = 0; i < 4; i++)
		colors[j][i] = ((3 - j) * colors[0][i] + j * colors[3][i]) / 3;

	cuts = CircleCuts(20);
/*	jcuts = CircleCuts(12);*/
	jcuts[0][0] = 0.;
	jcuts[0][1] = 1.;
	jcuts[1][0] = sin(tangent / 3.);
	jcuts[1][1] = cos(tangent / 3.);
	jcuts[2][0] = sin(tangent * 2. / 3.);
	jcuts[2][1] = cos(tangent * 2. / 3.);
	jcuts[3][0] = sin(tangent);
	jcuts[3][1] = cos(tangent);

	glPushAttrib(GL_POLYGON_BIT);
/*	glEnable(GL_CULL_FACE);*/
	glPushMatrix();
	glTranslated(pos[0], pos[1], pos[2]);
	if(irot)
		glMultMatrixd(irot);
	glScaled(radius, radius, radius);

	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv(colors[0]);
	glVertex3d(0., 0., 1.);
	glColor4ubv(colors[1]);
	for(i = 0; i <= 20; i++){
		int k = i % 20;
		glVertex3d(jcuts[1][0] * cuts[k][1], jcuts[1][0] * cuts[k][0], jcuts[1][1]);
	}
	glEnd();

	glBegin(GL_QUAD_STRIP);
	for(j = 1; j <= 2; j++) for(i = 0; i <= 20; i++){
		int k = i % 20;
		glColor4ubv(colors[j]);
		glVertex3d(jcuts[j][0] * cuts[k][1], jcuts[j][0] * cuts[k][0], jcuts[j][1]);
		glColor4ubv(colors[j+1]);
		glVertex3d(jcuts[j+1][0] * cuts[k][1], jcuts[j+1][0] * cuts[k][0], jcuts[j+1][1]);
	}
	glEnd();

	glPopMatrix();
	glPopAttrib();
}
