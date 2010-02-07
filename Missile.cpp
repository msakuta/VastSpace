#include "Missile.h"
#include "material.h"
extern "C"{
#include <clib/c.h>
#include <clib/suf/sufdraw.h>
#include <clib/gl/gldraw.h>
}

#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_orangeburn[] = {
	{0.1, COLOR32RGBA(255,255,191,255)},
	{0.15, COLOR32RGBA(255,255,31,191)},
	{0.45, COLOR32RGBA(255,127,31,95)},
	{0.3, COLOR32RGBA(255,31,0,63)},
};
const struct color_sequence cs_orangeburn = DEFINE_COLSEQ(cnl_orangeburn, (COLOR32)-1, 1.);




Missile::Missile(Entity *parent, float life, double damage) : st(parent, life, damage){
	WarSpace *ws = *parent->w;
	if(ws)
		pf = AddTefpolMovable3D(ws->tepl, pos, velo, avec3_000, &cs_orangeburn, TEP3_THICK | TEP3_ROUGH, cs_orangeburn.t);
}

const unsigned Missile::classid = registerClass("Missile", Conster<Missile>);
const char *Missile::classname()const{return "Missile";}


void Missile::anim(double dt){
	if(pf)
		MoveTefpol3D(pf, pos, avec3_000, cs_orangeburn.t, 0/*pf->docked*/);

	st::anim(dt);
}

void Missile::draw(wardraw_t *wd){
	static suf_t *suf;
	static suftex_t *suft;
	static bool init = false;
	if(!init) do{
		init = true;
		FILE *fp;
		suf = CallLoadSUF("missile.bin");
		if(!suf)
			break;
		CallCacheBitmap("missile_body.bmp", "models/missile_body.bmp", NULL, NULL);
		suft = AllocSUFTex(suf);
	} while(0);
	if(suf){
		static const double normal[3] = {0., 1., 0.};
		double x;
		double pyr[3];
		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

		glPushMatrix();
		gldTranslate3dv(this->pos);
		gldMultQuat(this->rot);
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
		gldScaled(1e-5);
		glScalef(-1, 1, -1);
		DecalDrawSUF(suf, SUF_ATR, NULL, suft, NULL, NULL);
		glPopMatrix();

		glPopAttrib();
	}
}

void Missile::drawtra(wardraw_t *wd){
	// redefine void
}
