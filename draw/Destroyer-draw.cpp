#include "../Destroyer.h"
#include "draw/material.h"
#include "effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


const double Destroyer::sufscale = .001;

void Destroyer::draw(wardraw_t *wd){
	static suf_t *sufs[2] = {NULL};
	static suftex_t *pst[2];
	static OpenGLState::weak_ptr<bool> init;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	struct TextureParams{
		Destroyer *p;
		WarDraw *wd;
		static void onBeginTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				p->wd->setAdditive(true);
				const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
				if(asb)
					asb->setIntensity(Vec3f(1. - (p->p->engineHeat - .6) * (p->p->engineHeat - .6) / (.6 * .6), p->p->engineHeat * 2, p->p->engineHeat * 3));
			}
		}
		static void onEndTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
	} tp = {this, wd};

	static int engineAtrIndex = -1;
	if(!init) do{
		for(int i = 0; i < 2; i++)
			free(sufs[i]);
		sufs[0] = CallLoadSUF("models/destroyer0.bin");
		sufs[1] = CallLoadSUF("models/destroyer0_bridge.bin");
		if(!sufs[0]) break;
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		for(int i = 0; i < 2; i++)
			CacheSUFMaterials(sufs[i]);
		for(int i = 0; i < 2; i++)
			pst[i] = gltestp::AllocSUFTex(sufs[i]);

		for(int i = 0; i < pst[0]->n; i++) if(sufs[0]->a[i].colormap && !strcmp(sufs[0]->a[i].colormap, "engine2.bmp")){
			engineAtrIndex = i;
			pst[0]->a[i].onBeginTexture = TextureParams::onBeginTextureEngine;
			pst[0]->a[i].onEndTexture = TextureParams::onEndTextureEngine;
		}

		init.create(*openGLState);
	} while(0);

	if(pst[0]){
		if(0 <= engineAtrIndex){
			pst[0]->a[engineAtrIndex].onBeginTextureData = &tp;
			pst[0]->a[engineAtrIndex].onEndTextureData = &tp;
		}
	}

	if(sufs[0]){
		static const GLdouble rotaxis[16] = {
			-1,0,0,0,
			0,1,0,0,
			0,0,-1,0,
			0,0,0,1,
		};
		Mat4d mat;

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		glPushMatrix();
		gldScaled(sufscale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufs[0], SUF_ATR, NULL, pst[0], NULL, NULL);

		DecalDrawSUF(sufs[1], SUF_ATR, NULL, pst[1], NULL, NULL);
		glScalef(-1, 1, 1);
		glFrontFace(GL_CW);
//		glCullFace(GL_FRONT);
		DecalDrawSUF(sufs[1], SUF_ATR, NULL, pst[1], NULL, NULL);
		glScalef(1, -1, 1);
		glFrontFace(GL_CCW);
//		glCullFace(GL_BACK);
		DecalDrawSUF(sufs[1], SUF_ATR, NULL, pst[1], NULL, NULL);
		glScalef(-1, 1, 1);
		glFrontFace(GL_CW);
//		glCullFace(GL_FRONT);
		DecalDrawSUF(sufs[1], SUF_ATR, NULL, pst[1], NULL, NULL);

		glPopMatrix();

		glPopMatrix();
		glPopAttrib();
	}
}

void Destroyer::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;
	static const Vec3d engines[] = {
		Vec3d(0, 0, 180) * sufscale,
		Vec3d(28, 22, 180) * sufscale,
		Vec3d(-28, 22, 180) * sufscale,
		Vec3d(28, -22, 180) * sufscale,
		Vec3d(-28, -22, 180) * sufscale,
	};
	for(int i = 0; i < numof(engines); i++)
		drawCapitalBlast(wd, engines[i], .02);
//	drawShield(wd);
}


void WireDestroyer::draw(wardraw_t *wd){
	static suf_t *sufbase;
	static suf_t *sufwheel;
	static suf_t *sufbit;
	static suftex_t *pst;
	static bool init = false;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, -1);

	if(!init) do{
		sufbase = CallLoadSUF("models/wiredestroyer0.bin");
		if(!sufbase) break;
		sufwheel = CallLoadSUF("models/wiredestroyer_wheel0.bin");
		if(!sufwheel) break;
		sufbit = CallLoadSUF("models/wirebit0.bin");
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

