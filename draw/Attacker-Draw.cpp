/** \file
 * \brief Implements Attacker's drawing methods.
 */
#include "../Attacker.h"
#include "../material.h"
#include "effects.h"
#include "draw/WarDraw.h"
#include "ShadowMap.h"
#include "motion.h"
extern "C"{
#include <clib/gl/gldraw.h>
}

struct AttackerTextureParams{
	Attacker *p;
	WarDraw *wd;
};

void Attacker::draw(wardraw_t *wd){
	static suf_t *sufbase, *sufbridge;
	static suftex_t *pst, *pstbridge;
	static bool init = false;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	if(!init) do{
		sufbase = CallLoadSUF("models/attack_body.bin");
		if(!sufbase) break;
		sufbridge = CallLoadSUF("models/attack_bridge.bin");
//		CallCacheBitmap("bricks.bmp", "bricks.bmp", NULL, NULL);
//		CallCacheBitmap("runway.bmp", "runway.bmp", NULL, NULL);
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("attacker_engine.bmp", "models/attacker_engine_br.bmp", &stp, "models/attacker_engine.bmp", NULL);
		CacheSUFMaterials(sufbase);
		CacheSUFMaterials(sufbridge);
		pst = AllocSUFTex(sufbase);
		for(int i = 0; i < pst->n; i++) if(pst->a[i].tex[1]){
			pst->a[i].onBeginTexture = onBeginTexture;
			pst->a[i].onEndTexture = onEndTexture;
		}
		pstbridge = AllocSUFTex(sufbridge);
		init = true;
	} while(0);

	if(sufbase){
		AttackerTextureParams atp;
		atp.p = this;
		atp.wd = wd;

		// This values could change every frame, so we assign them here.
		for(int i = 0; i < pst->n; i++) if(pst->a[i].tex[1]){
			pst->a[i].onBeginTextureData = &atp;
			pst->a[i].onEndTextureData = &atp;
		}

		double scale = .001;
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
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
		gldCache c;
		c.valid = 0;

		glPushMatrix();
		glScaled(-scale, scale, -scale);
		DecalDrawSUF(sufbase, SUF_ATR, &c, pst, NULL, NULL);
		glPushMatrix();
		glScaled(-1,1,1);
		glFrontFace(GL_CW);
		DecalDrawSUF(sufbase, SUF_ATR, &c, pst, NULL, NULL);
		glFrontFace(GL_CCW);
		glPopMatrix();
		DecalDrawSUF(sufbridge, SUF_ATR, &c, pstbridge, NULL, NULL);
		glPopMatrix();

		glPopMatrix();
		glPopAttrib();
	}
}

void Attacker::drawOverlay(wardraw_t *){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.15,  1.0);
	glVertex2d(-.4,  1.0);
	glVertex2d(-.5,  0.9);
	glVertex2d(-.5,  0.2);
	glVertex2d(-.9, -0.5);
	glVertex2d(-.9, -0.7);
	glVertex2d(-.5, -0.8);
	glVertex2d(-.5, -0.9);
	glVertex2d(-.2, -1.0);
	glVertex2d( .2, -1.0);
	glVertex2d( .5, -0.9);
	glVertex2d( .5, -0.8);
	glVertex2d( .9, -0.7);
	glVertex2d( .9, -0.5);
	glVertex2d( .5,  0.2);
	glVertex2d( .5,  0.9);
	glVertex2d( .4,  1.0);
	glVertex2d( .15,  1.0);
	glVertex2d( .15,  0.3);
	glVertex2d(-.15,  0.3);
	glEnd();
}


void Attacker::onBeginTexture(void *pv){
	AttackerTextureParams *p = (AttackerTextureParams*)pv;
	if(p && p->wd && p->wd->shadowMap){
		p->wd->shadowMap->setAdditive(true);
		const AdditiveShaderBind *asb = p->wd->shadowMap->getAdditive();
		asb->setIntensity(p->p->engineHeat);
	}
}

void Attacker::onEndTexture(void *pv){
	AttackerTextureParams *p = (AttackerTextureParams*)pv;
	if(p && p->wd && p->wd->shadowMap)
		p->wd->shadowMap->setAdditive(false);
}
