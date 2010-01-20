#include "Destroyer.h"
#include "material.h"

const unsigned Destroyer::classid = registerClass("destroyer", Conster<Destroyer>);
const char *Destroyer::classname()const{return "destroyer";}
const char *Destroyer::dispname()const{return "Destroyer";}


struct hardpoint_static *Destroyer::hardpoints = NULL;
int Destroyer::nhardpoints = 0;

Destroyer::Destroyer(WarField *aw) : st(w){
	init();
	for(int i = 0; i < nhardpoints; i++){
		turrets[i] = new LTurret(this, &hardpoints[i]);
		if(aw)
			aw->addent(turrets[i]);
	}
}

void Destroyer::init(){
	st::init();
	if(!hardpoints){
		hardpoints = hardpoint_static::load("Destroyer.hps", nhardpoints);
	}
	turrets = new ArmBase*[nhardpoints];
	mass = 1e6;
}

double Destroyer::hitradius(){return .25;}

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

void Destroyer::draw(wardraw_t *wd){
	static suf_t *sufbase;
	static suftex_t *pst;
	static bool init = false;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, -1);

	if(!init) do{
		sufbase = CallLoadSUF("destroyer0.bin");
		if(!sufbase) break;
//		cache_bridge();
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

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

#if 0
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
	}
}

double Destroyer::maxhealth()const{return 100000.;}
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
