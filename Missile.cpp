#include "Missile.h"
#include "material.h"
extern "C"{
#include <clib/suf/sufdraw.h>
#include <clib/gl/gldraw.h>
}

const unsigned Missile::classid = registerClass("Missile", Conster<Missile>);
const char *Missile::classname()const{return "Missile";}


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
