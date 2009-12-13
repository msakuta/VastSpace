#include "Assault.h"
#include "Beamer.h"
#include "judge.h"
#include "player.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}


#define BEAMER_SCALE .0002




#define SQRT2P2 (M_SQRT2/2.)

const struct hardpoint_static Assault::hardpoints[5] = {
	hardpoint_static(Vec3d(.000, 50 * BEAMER_SCALE, -110 * BEAMER_SCALE), Quatd(0,0,0,1), "Top Turret", 0),
	hardpoint_static(Vec3d(.000, -50 * BEAMER_SCALE, -110 * BEAMER_SCALE), Quatd(0,0,1,0), "Bottom Turret", 0),
	hardpoint_static(Vec3d(40 * BEAMER_SCALE,  .000, -225 * BEAMER_SCALE), Quatd(0,0,-SQRT2P2,SQRT2P2), "Right Turret", 0),
	hardpoint_static(Vec3d(-40 * BEAMER_SCALE,  .000, -225 * BEAMER_SCALE), Quatd(0,0,SQRT2P2,SQRT2P2), "Left Turret", 0),
	hardpoint_static(Vec3d(0, 0, 0), Quatd(0,0,0,1), "Shield Generator", 0),
};



suf_t *Assault::sufbase = NULL;

Assault::Assault(WarField *aw) : st(aw){
	init();
	for(int i = 0; i < 4; i++){
		aw->addent(turrets[i] = (i % 2 ? new MTurret(this, &hardpoints[i]) : new GatlingTurret(this, &hardpoints[i])));
	}
}

const char *Assault::idname()const{
	return "assault";
}

const char *Assault::classname()const{
	return "Sabre class";
}

void Assault::anim(double dt){
	st::anim(dt);
	for(int i = 0; i < 4; i++) if(turrets[i])
		turrets[i]->align();
}

void Assault::postframe(){
	st::postframe();
	for(int i = 0; i < 4; i++) if(turrets[i] && turrets[i]->w != w)
		turrets[i] = NULL;
}

void Assault::draw(wardraw_t *wd){
	Assault *const p = this;
	static int init = 0;
	static suftex_t *pst;
	if(!w)
		return;

	/* cull object */
/*	if(beamer_cull(this, wd))
		return;*/
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / maxhealth(), .1, shieldAmount / maxshield(), capacitor / maxenergy());

	if(init == 0) do{
		sufbase = CallLoadSUF("assault.bin");
		if(!sufbase) break;
		Beamer::cache_bridge();
		pst = AllocSUFTex(sufbase);
		init = 1;
	} while(0);
	if(sufbase){
		static const double normal[3] = {0., 1., 0.};
		double scale = BEAMER_SCALE;
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
/*		glPushAttrib(GL_TEXTURE_BIT);*/
/*		DrawSUF(beamer_s.sufbase, SUF_ATR, &g_gldcache);*/
		DecalDrawSUF(sufbase, SUF_ATR, NULL, pst, NULL, NULL);
/*		DecalDrawSUF(&suf_beamer, SUF_ATR, &g_gldcache, beamer_s.tex, pf->sd, &beamer_s);*/
/*		glPopAttrib();*/
		glPopMatrix();

/*		for(i = 0; i < numof(pf->turrets); i++)
			mturret_draw(&pf->turrets[i], pt, wd);*/
		glPopMatrix();
	}
}

void Assault::drawtra(wardraw_t *wd){
	drawCapitalBlast(wd, Vec3d(0,-0.003,.06));
	drawShield(wd);
}

double Assault::maxhealth()const{return 30000.;}

int Assault::armsCount()const{return numof(turrets);}

const ArmBase *Assault::armsGet(int i)const{
	if(i < 0 || armsCount() <= i)
		return NULL;
	return turrets[i];
}

void Assault::attack(Entity *target){
	for(int i = 0; i < numof(turrets); i++) if(turrets[i])
		turrets[i]->attack(target);
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
	if(!ppl || !ppl->selected)
		return 0;
	glwAppend(new GLWarms("Arms", ppl->selected));
	return 0;
}

