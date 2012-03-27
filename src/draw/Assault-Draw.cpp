/** \file
 * \brief Visual implementation of Assault class.
 */
#include "../Assault.h"
#include "../Beamer.h"
#include "Player.h"
#include "draw/material.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
#include "draw/WarDraw.h"
#include "draw/blackbody.h"
#include "draw/mqoadapt.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


#define BEAMER_SCALE .0002



suf_t *Assault::sufbase = NULL;


void Assault::draw(wardraw_t *wd){
	Assault *const p = this;
	static OpenGLState::weak_ptr<bool> init;
	static suftex_t *pst;
	static GLuint list = 0;
	static Model *model = NULL;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .1, shieldAmount / maxshield(), capacitor / maxenergy());

	struct TextureParams{
		Assault *p;
		WarDraw *wd;
		static void onInitedTextureEngineValve(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Vec4f(blackbodyEmit(p->p->engineHeat * 13000.), 1.));
			}
		}
		static void onBeginTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd){
				p->wd->setAdditive(true);
				const AdditiveShaderBind *asb = p->wd->getAdditiveShaderBind();
				if(asb)
					asb->setIntensity(Vec4f(blackbodyEmit(p->p->engineHeat * 13000.), 1.));
			}
		}
		static void onEndTextureEngine(void *pv){
			TextureParams *p = (TextureParams*)pv;
			if(p && p->wd)
				p->wd->setAdditive(false);
		}
	} tp = {this, wd};

	static int engineValveAtrIndex = -1, engineAtrIndex = -1;
	if(!init) do{
		model = LoadMQOModel("models/assault.mqo");

		if(model && model->sufs[0]){
			for(int i = 0; i < model->tex[0]->n; i++) if(!strcmp(model->sufs[0]->a[i].colormap, "engine.bmp")){
				engineValveAtrIndex = i;
				model->tex[0]->a[i].onInitedTexture = TextureParams::onInitedTextureEngineValve;
			}
			else if(!strcmp(model->sufs[0]->a[i].colormap, "enginenozzle.png")){
				engineAtrIndex = i;
				model->tex[0]->a[i].onBeginTexture = TextureParams::onBeginTextureEngine;
				model->tex[0]->a[i].onEndTexture = TextureParams::onEndTextureEngine;
			}
		}

		init.create(*openGLState);
	} while(0);

	if(model){
		if(model && model->tex[0]){
			if(0 <= engineValveAtrIndex){
				model->tex[0]->a[engineValveAtrIndex].onInitedTextureData = &tp;
			}
			if(0 <= engineAtrIndex){
				model->tex[0]->a[engineAtrIndex].onBeginTextureData = &tp;
				model->tex[0]->a[engineAtrIndex].onEndTextureData = &tp;
			}
		}

		double scale = BEAMER_SCALE;
		Mat4d mat;

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);
/*		if(!list){
		glNewList(list = glGenLists(1), GL_COMPILE);*/
#if 0
		for(int i = 0; i < nhitboxes; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(hitboxes[i].org);
			rot = hitboxes[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox hb(mat.vp3(hitboxes[i].org), this->rot * hitboxes[i].rot, hitboxes[i].sc);
			hitbox_draw(this, hitboxes[i].sc/*, jHitBoxPlane(hb, Vec3d(0,0,0), Vec3d(0,0,1))*/);
			glPopMatrix();
		}
#endif

		glScaled(-scale, scale, -scale);
		DrawMQOPose(model, NULL);
		glPopMatrix();
	}
}

void Assault::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	if(cull(wd))
		return;
	drawNavlights(wd, navlights);
	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);
	drawShield(wd);
}

void Assault::drawOverlay(wardraw_t *){
	glCallList(disp);
}







class GLWarms : public GLwindowSizeable{
	Entity *a;
public:
	typedef GLwindowSizeable st;
	GLWarms(const char *title, Entity *a);
	virtual void draw(GLwindowState &ws, double t);
	virtual void postframe();
};

GLWarms::GLWarms(const char *atitle, Entity *aa) : st(atitle), a(aa){
	xpos = 120;
	ypos = 40;
	width = 200;
	height = 200;
	flags |= GLW_CLOSE;
}

void GLWarms::draw(GLwindowState &ws, double t){
	if(!a)
		return;
	glColor4f(0,1,1,1);
	glwpos2d(xpos, ypos + (2) * getFontHeight());
	glwprintf(a->classname());
	glColor4f(1,1,0,1);
	glwpos2d(xpos, ypos + (3) * getFontHeight());
	glwprintf("%lg / %lg", a->health, a->maxhealth());
	for(int i = 0; i < a->armsCount(); i++){
		const ArmBase *arm = a->armsGet(i);
		glColor4f(0,1,1,1);
		glwpos2d(xpos, ypos + (4 + 2 * i) * getFontHeight());
		glwprintf(arm->hp->name);
		glColor4f(1,1,0,1);
		glwpos2d(xpos, ypos + (5 + 2 * i) * getFontHeight());
		glwprintf(arm ? arm->descript() : "N/A");
	}
}

void GLWarms::postframe(){
	if(a && !a->w)
		a = NULL;
}

int cmd_armswindow(int argc, char *argv[], void *pv){
	Player *ppl = (Player*)pv;
	if(!ppl || ppl->selected.empty())
		return 0;
	glwAppend(new GLWarms("Arms", *ppl->selected.begin()));
	return 0;
}
