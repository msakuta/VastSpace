#include "../Destroyer.h"
#include "draw/material.h"
#include "effects.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


void Destroyer::draw(wardraw_t *wd){
	static suf_t *sufbase = NULL;
	static suf_t *sufbridge = NULL;
	static suftex_t *pst, *pstbridge;
	static OpenGLState::weak_ptr<bool> init;

	draw_healthbar(this, wd, health / maxhealth(), .3, -1, capacitor / maxenergy());

	if(wd->vw->gc->cullFrustum(pos, hitradius()))
		return;

	if(!init) do{
		free(sufbase);
		free(sufbridge);
		sufbase = CallLoadSUF("models/destroyer0.bin");
		sufbridge = CallLoadSUF("models/destroyer0_bridge.bin");
		if(!sufbase) break;
		suftexparam_t stp;
		stp.flags = STP_MAGFIL | STP_MINFIL | STP_ENV;
		stp.magfil = GL_LINEAR;
		stp.minfil = GL_LINEAR;
		stp.env = GL_ADD;
		stp.mipmap = 0;
		CallCacheBitmap5("engine2.bmp", "engine2br.bmp", &stp, "engine2.bmp", NULL);
		CacheSUFMaterials(sufbase);
		CacheSUFMaterials(sufbridge);
		pst = gltestp::AllocSUFTex(sufbase);
		pstbridge = gltestp::AllocSUFTex(sufbridge);
		init.create(*openGLState);
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

		glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		glPushMatrix();
		glScaled(scale, scale, scale);
		glMultMatrixd(rotaxis);
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);

		DecalDrawSUF(sufbridge, SUF_ATR, NULL, pstbridge, NULL, NULL);
		glScalef(-1, 1, 1);
		glCullFace(GL_FRONT);
//		glCullFace(GL_BACK);
		DecalDrawSUF(sufbridge, SUF_ATR, NULL, pstbridge, NULL, NULL);
		glScalef(1, -1, 1);
		glCullFace(GL_BACK);
//		glCullFace(GL_FRONT);
		DecalDrawSUF(sufbridge, SUF_ATR, NULL, pstbridge, NULL, NULL);
		glScalef(-1, 1, 1);
		glCullFace(GL_FRONT);
//		glCullFace(GL_BACK);
		DecalDrawSUF(sufbridge, SUF_ATR, NULL, pstbridge, NULL, NULL);

		glPopMatrix();

		glPopMatrix();
		glPopAttrib();
	}
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

