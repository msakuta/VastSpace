#include "Destroyer.h"
#include "material.h"
#include "judge.h"
#include "serial_util.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

const unsigned Destroyer::classid = registerClass("Destroyer", Conster<Destroyer>);
const unsigned Destroyer::entityid = registerEntity("Destroyer", Constructor<Destroyer>);
const char *Destroyer::classname()const{return "Destroyer";}
const char *Destroyer::dispname()const{return "Destroyer";}


struct hardpoint_static *Destroyer::hardpoints = NULL;
int Destroyer::nhardpoints = 0;
struct hitbox Destroyer::hitboxes[] = {
	hitbox(Vec3d(0., 0., -.058), Quatd(0,0,0,1), Vec3d(.051, .032, .190)),
	hitbox(Vec3d(0., 0., .193), Quatd(0,0,0,1), Vec3d(.051, .045, .063)),
	hitbox(Vec3d(.0, -.06, .005), Quatd(0,0,0,1), Vec3d(.015, .030, .018)),
	hitbox(Vec3d(.0, .06, .005), Quatd(0,0,0,1), Vec3d(.015, .030, .018)),
};
const int Destroyer::nhitboxes = numof(Destroyer::hitboxes);


Destroyer::Destroyer(WarField *aw) : st(aw){
	init();
	for(int i = 0; i < nhardpoints; i++){
		turrets[i] = 1&&i % 3 != 0 ? (LTurretBase*)new LTurret(this, &hardpoints[i]) : (LTurretBase*)new LMissileTurret(this, &hardpoints[i]);
		if(aw)
			aw->addent(turrets[i]);
	}
}

void Destroyer::static_init(){
	if(!hardpoints){
		hardpoints = hardpoint_static::load("Destroyer.hps", nhardpoints);
	}
}

void Destroyer::init(){
	static_init();
	st::init();
	turrets = new ArmBase*[nhardpoints];
	mass = 1e6;
}

void Destroyer::serialize(SerializeContext &sc){
	st::serialize(sc);
	for(int i = 0; i < nhardpoints; i++)
		sc.o << turrets[i];
}

void Destroyer::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	for(int i = 0; i < nhardpoints; i++)
		sc.i >> turrets[i];
}

double Destroyer::hitradius()const{return .25;}

void Destroyer::anim(double dt){
	st::anim(dt);
	for(int i = 0; i < nhardpoints; i++) if(turrets[i])
		turrets[i]->align();
}

void Destroyer::postframe(){
	st::postframe();
	for(int i = 0; i < nhardpoints; i++) if(turrets[i] && turrets[i]->w != w)
		turrets[i] = NULL;
}

int Destroyer::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
#if 0
	if(0 < p->shieldAmount){
		Vec3d hitpos;
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, &hitpos)){
			if(retp) *retp = hitpos;
			if(retn) *retn = (hitpos - pos).norm();
			return 1000; /* something quite unlikely to reach */
		}
	}
#endif
	for(n = 0; n < nhitboxes; n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		for(i = 0; i < 3; i++)
			sc[i] = hitboxes[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
	}
	return reti;
}

void Destroyer::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	rot = this->rot;
	pos = rot.trans(Vec3d(0, .08, .0)) + this->pos;
}

void Destroyer::draw(wardraw_t *wd){
	static suf_t *sufbase;
	static suftex_t *pst;
	static bool init = false;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, -1);

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	if(!init) do{
		sufbase = CallLoadSUF("destroyer0.bin");
		if(!sufbase) break;
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		CacheSUFMaterials(sufbase);
		pst = AllocSUFTex(sufbase);
		init = true;
	} while(0);

	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = .001;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 1
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, hitboxes[i].sc);
			glPopMatrix();
		}
#endif

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
		glPopMatrix();

		glPopMatrix();
		glPopAttrib();
	}
}

extern void smokedraw(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);
extern void debrigib(const struct tent3d_line_callback *pl, const struct tent3d_line_drawdata *dd, void *pv);

int Destroyer::takedamage(double damage, int hitpart){
	Destroyer *p = this;
	struct tent3d_line_list *tell = w->getTeline3d();
	int ret = 1;

//	playWave3D("hit.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
	if(0 < p->health && p->health - damage <= 0){
		int i;
		ret = 0;
		WarSpace *ws = *w;
		if(ws){

			if(ws->gibs) for(i = 0; i < 128; i++){
				double pos[3], velo[3] = {0}, omg[3];
				/* gaussian spread is desired */
				for(int j = 0; j < 6; j++)
					velo[j / 2] += .025 * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[0] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[1] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				omg[2] = M_PI * 2. * (drseq(&w->rs) - .5 + drseq(&w->rs) - .5);
				VECCPY(pos, this->pos);
				for(int j = 0; j < 3; j++)
					pos[j] += hitradius() * (drseq(&w->rs) - .5);
				AddTelineCallback3D(ws->gibs, pos, velo, .010, NULL, omg, NULL, debrigib, NULL, TEL3_QUAT | TEL3_NOLINE, 15. + drseq(&w->rs) * 5.);
			}

			/* smokes */
			for(i = 0; i < 64; i++){
				double pos[3];
				COLOR32 col = 0;
				VECCPY(pos, p->pos);
				pos[0] += .3 * (drseq(&w->rs) - .5);
				pos[1] += .3 * (drseq(&w->rs) - .5);
				pos[2] += .3 * (drseq(&w->rs) - .5);
				col |= COLOR32RGBA(rseq(&w->rs) % 32 + 127,0,0,0);
				col |= COLOR32RGBA(0,rseq(&w->rs) % 32 + 127,0,0);
				col |= COLOR32RGBA(0,0,rseq(&w->rs) % 32 + 127,0);
				col |= COLOR32RGBA(0,0,0,191);
	//			AddTeline3D(w->tell, pos, NULL, .035, NULL, NULL, NULL, col, TEL3_NOLINE | TEL3_GLOW | TEL3_INVROTATE, 60.);
				AddTelineCallback3D(ws->tell, pos, NULL, .07, NULL, NULL, NULL, smokedraw, (void*)col, TEL3_INVROTATE | TEL3_NOLINE, 60.);
			}

			{/* explode shockwave thingie */
				static const double pyr[3] = {M_PI / 2., 0., 0.};
				amat3_t ort;
				Vec3d dr, v;
				Quatd q;
				amat4_t mat;
				double p;
	/*			w->vft->orientation(w, &ort, &pt->pos);
				VECCPY(dr, &ort[3]);*/
				dr = vec3_001;

				/* half-angle formula of trigonometry replaces expensive tri-functions to square root */
				q[3] = sqrt((dr[2] + 1.) / 2.) /*cos(acos(dr[2]) / 2.)*/;

				v = vec3_001.vp(dr);
				p = sqrt(1. - q[3] * q[3]) / VECLEN(v);
				q = v * p;

				AddTeline3D(tell, this->pos, NULL, 5., q, NULL, NULL, COLOR32RGBA(255,191,63,255), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_QUAT, 2.);
				AddTeline3D(tell, this->pos, NULL, 3., NULL, NULL, NULL, COLOR32RGBA(255,255,255,127), TEL3_EXPANDISK | TEL3_NOLINE | TEL3_INVROTATE, 2.);
			}
		}
//		playWave3D("blast.wav", pt->pos, w->pl->pos, w->pl->pyr, 1., .01, w->realtime);
		p->w = NULL;
	}
	p->health -= damage;
	return ret;
}

double Destroyer::maxhealth()const{return 100000./1;}

int Destroyer::armsCount()const{
	return nhardpoints;
}

const ArmBase *Destroyer::armsGet(int i)const{
	if(i < 0 || armsCount() <= i)
		return NULL;
	return turrets[i];
}

double Destroyer::maxenergy()const{return getManeuve().capacity;}

const Warpable::maneuve &Destroyer::getManeuve()const{
	static const struct Warpable::maneuve beamer_mn = {
		.05, /* double accel; */
		.1, /* double maxspeed; */
		.1, /* double angleaccel; */
		.2, /* double maxanglespeed; */
		150000., /* double capacity; [MJ] */
		300., /* double capacitor_gen; [MW] */
	};
	return beamer_mn;
}










struct hitbox WireDestroyer::hitboxes[] = {
	hitbox(Vec3d(0., 0., -.058), Quatd(0,0,0,1), Vec3d(.051, .032, .190)),
	hitbox(Vec3d(0., 0., .193), Quatd(0,0,0,1), Vec3d(.051, .045, .063)),
	hitbox(Vec3d(.0, -.06, .065), Quatd(0,0,0,1), Vec3d(.015, .030, .018)),
	hitbox(Vec3d(.0, .06, .065), Quatd(0,0,0,1), Vec3d(.015, .030, .018)),
	hitbox(Vec3d(.0, .0, .0), Quatd(0,0,0,1), Vec3d(.07, .070, .02)),
};
const int WireDestroyer::nhitboxes = numof(hitboxes);

WireDestroyer::WireDestroyer(WarField *aw) : st(aw), wirephase(0), wireomega(0), wirelength(2.){
	init();
	mass = 1e6;
}


const unsigned WireDestroyer::classid = registerClass("WireDestroyer", Conster<WireDestroyer>);
const unsigned WireDestroyer::entityid = registerEntity("WireDestroyer", Constructor<WireDestroyer>);
const char *WireDestroyer::classname()const{return "WireDestroyer";}
const char *WireDestroyer::dispname()const{return "Wire Destroyer";}

void WireDestroyer::serialize(SerializeContext &sc){
	st::serialize(sc);
}

void WireDestroyer::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
}

double WireDestroyer::hitradius()const{return .3;}
double WireDestroyer::maxhealth()const{return 100000.;}
double WireDestroyer::maxenergy()const{return 10000.;}

static int wire_hit_callback(const struct otjEnumHitSphereParam *param, Entity *pt){
	const Vec3d *src = param->src;
	const Vec3d *dir = param->dir;
	double dt = param->dt;
	double rad = param->rad;
	Vec3d *retpos = param->pos;
	Vec3d *retnorm = param->norm;
	void *hint = param->hint;
	WireDestroyer *pb = ((WireDestroyer**)hint)[1];
	double hitrad = .02;
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
	if(!jHitSphere(pt->pos, pt->hitradius() + hitrad, *param->src, *param->dir, dt))
		return 0;
/*	{
		ret = pt->tracehit(pb->pos, pb->velo, hitrad, dt, NULL, &pos, &nh);
		if(!ret)
			return 0;
		else if(rethitpart)
			*rethitpart = ret;
	}*/

/*	{
		pb->pos += pb->velo * dt;
		if(retpos)
			*retpos = pos;
		if(retnorm)
			*retnorm = nh;
	}*/
	return ret;
}

void WireDestroyer::anim(double dt){
	st::anim(dt);
	wireomega = 6. * M_PI;

	int subwires = 8;
	if(WarSpace *ws = *w) for(int n = 0; n < subwires; n++){
		Mat4d mat;
		transform(mat);
		for(int i = 0; i < 2; i++){
			Mat4d rot = mat.rotz(wirephase + (wireomega * dt) * n / subwires);
			Vec3d src = rot.vp3(Vec3d(.07 * (i * 2 - 1), 0, 0));
			Vec3d dst = rot.vp3(Vec3d(wirelength * (i * 2 - 1), 0, 0));
			Vec3d dir = (dst - src).norm();
			struct otjEnumHitSphereParam param;
			void *hint[3];
			Entity *pt;
			Vec3d pos, nh;
			int hitpart = 0;
			hint[0] = w;
			hint[1] = this;
			hint[2] = &hitpart;
			param.root = ws->otroot;
			param.src = &src;
			param.dir = &dir;
			param.dt = wirelength;
			param.rad = 0.01;
			param.pos = &pos;
			param.norm = &nh;
			param.flags = OTJ_CALLBACK;
			param.callback = wire_hit_callback;
			param.hint = hint;
			for(pt = ws->ot ? (otjEnumHitSphere(&param)) : ws->el; pt; pt = ws->ot ? NULL : pt->next){
				sufindex pi;
				double damage = 1000.;

				if(!ws->ot && !wire_hit_callback(&param, pt))
					continue;

				if(pt->w == w) if(!pt->takedamage(damage, hitpart)){
					extern int wire_kills;
					wire_kills++;
				}
			}while(0);
		}
	}
	wirephase += wireomega * dt;
}

void WireDestroyer::draw(wardraw_t *wd){
	static suf_t *sufbase;
	static suf_t *sufwheel;
	static suf_t *sufbit;
	static suftex_t *pst;
	static bool init = false;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, -1);

	if(!init) do{
		sufbase = CallLoadSUF("wiredestroyer0.bin");
		if(!sufbase) break;
		sufwheel = CallLoadSUF("wiredestroyer_wheel0.bin");
		if(!sufwheel) break;
		sufbit = CallLoadSUF("wirebit0.bin");
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		CacheSUFMaterials(sufbase);
		pst = AllocSUFTex(sufbase);
		init = true;
	} while(0);

	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = .001;
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		if(!wd->vw->gc->cullFrustum(pos, hitradius())){

	#if 1
			for(int i = 0; i < nhitboxes; i++){
				Mat4d rot;
				glPushMatrix();
				gldTranslate3dv(hitboxes[i].org);
				rot = hitboxes[i].rot.tomat4();
				glMultMatrixd(rot);
				hitbox_draw(this, hitboxes[i].sc);
				glPopMatrix();
			}
	#endif

			glPushMatrix();
			gldScaled(scale);
			glMultMatrixd(rotaxis);
			DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
			glRotated(wirephase * deg_per_rad, 0, 0, -1);
			DecalDrawSUF(sufwheel, SUF_ATR, NULL, pst, NULL, NULL);
			glPopMatrix();
		}

		for(int i = 0; i < 2; i++){
			glPushMatrix();
			glRotated(wirephase * deg_per_rad, 0, 0, 1);
			glTranslated(wirelength * (i * 2 - 1), 0, 0);
			gldScaled(scale);
			glMultMatrixd(rotaxis);
			DrawSUF(sufbit, SUF_ATR, NULL);
			glPopMatrix();
		}

		glPopMatrix();
		glPopAttrib();
	}
}

void WireDestroyer::drawtra(wardraw_t *wd){
	Mat4d mat;
	transform(mat);
	for(int i = 0; i < 2; i++){
		Mat4d rot = mat.rotz(wirephase);
		glColor4f(1,.5,.5,1);
		gldBeam(wd->vw->pos, rot.vp3(Vec3d(.07 * (i * 2 - 1), 0, 0)), rot.vp3(Vec3d(wirelength * (i * 2 - 1), 0, 0)), .01);
	}
}

int WireDestroyer::tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn){
	double sc[3];
	double best = dt, retf;
	int reti = 0, i, n;
#if 0
	if(0 < p->shieldAmount){
		Vec3d hitpos;
		if(jHitSpherePos(pos, BEAMER_SHIELDRAD + rad, src, dir, dt, ret, &hitpos)){
			if(retp) *retp = hitpos;
			if(retn) *retn = (hitpos - pos).norm();
			return 1000; /* something quite unlikely to reach */
		}
	}
#endif
	for(n = 0; n < nhitboxes; n++){
		Vec3d org;
		Quatd rot;
		org = this->rot.itrans(hitboxes[n].org) + this->pos;
		rot = this->rot * hitboxes[n].rot;
		for(i = 0; i < 3; i++)
			sc[i] = hitboxes[n].sc[i] + rad;
		if((jHitBox(org, sc, rot, src, dir, 0., best, &retf, retp, retn)) && (retf < best)){
			best = retf;
			if(ret) *ret = retf;
			reti = i + 1;
		}
	}
	return reti;
}

void WireDestroyer::cockpitView(Vec3d &pos, Quatd &rot, int seatid)const{
	rot = this->rot;
	pos = rot.trans(Vec3d(0, .08, .05)) + this->pos;
}

const Warpable::maneuve &WireDestroyer::getManeuve()const{
	static const struct Warpable::maneuve beamer_mn = {
		.05, /* double accel; */
		.1, /* double maxspeed; */
		.1, /* double angleaccel; */
		.2, /* double maxanglespeed; */
		150000., /* double capacity; [MJ] */
		300., /* double capacitor_gen; [MW] */
	};
	return beamer_mn;
}
