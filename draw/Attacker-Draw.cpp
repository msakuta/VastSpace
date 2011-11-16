/** \file
 * \brief Implements Attacker's drawing methods.
 */
#include "../Attacker.h"
#include "draw/material.h"
#include "effects.h"
#include "draw/WarDraw.h"
#include "draw/ShaderBind.h"
#include "draw/OpenGLState.h"
#include "motion.h"
extern "C"{
#include <clib/gl/gldraw.h>
#include <clib/suf/sufvbo.h>
}

struct AttackerTextureParams{
	Attacker *p;
	WarDraw *wd;
};

void Attacker::draw(wardraw_t *wd){
	static suf_t *sufbase, *sufbridge;
	static VBO *vbo[2] = {NULL};
	static suftex_t *pst, *pstbridge;
	static OpenGLState::weak_ptr<bool> init;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	if(!init) do{
		free(sufbase);
		free(sufbridge);
		sufbase = CallLoadSUF("models/attack_body.bin");
		if(!sufbase) break;
		sufbridge = CallLoadSUF("models/attack_bridge.bin");
		vbo[0] = CacheVBO(sufbase);
		vbo[1] = CacheVBO(sufbridge);
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
		pst = gltestp::AllocSUFTex(sufbase);
		for(int i = 0; i < pst->n; i++) if(pst->a[i].tex[1]){
			pst->a[i].onBeginTexture = onBeginTexture;
			pst->a[i].onEndTexture = onEndTexture;
		}
		pstbridge = gltestp::AllocSUFTex(sufbridge);
		init.create(*openGLState);
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
		if(vbo[0])
			DrawVBO(vbo[0], wd->shadowmapping ? 0 : SUF_ATR, pst);
		else
			DecalDrawSUF(sufbase, SUF_ATR, &c, pst, NULL, NULL);
		glPushMatrix();
		glScaled(-1,1,1);
		glFrontFace(GL_CW);
		if(vbo[0])
			DrawVBO(vbo[0], wd->shadowmapping ? 0 : SUF_ATR, pst);
		else
			DecalDrawSUF(sufbase, SUF_ATR, &c, pst, NULL, NULL);
		glFrontFace(GL_CCW);
		glPopMatrix();
		if(vbo[1])
			DrawVBO(vbo[1], wd->shadowmapping ? 0 : SUF_ATR, pstbridge);
		else
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
	if(p && p->wd){
		p->wd->setAdditive(true);
		const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
		if(asb)
			asb->setIntensity(Vec3f(1. - (p->p->engineHeat - .6) * (p->p->engineHeat - .6) / (.6 * .6), p->p->engineHeat * 2, p->p->engineHeat * 3));
	}
}

void Attacker::onEndTexture(void *pv){
	AttackerTextureParams *p = (AttackerTextureParams*)pv;
	if(p && p->wd)
		p->wd->setAdditive(false);
}