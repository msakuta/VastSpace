#include "Assault.h"
#include "Beamer.h"
#include "judge.h"
#include "player.h"
#include "serial_util.h"
extern "C"{
#include <clib/mathdef.h>
#include <clib/gl/gldraw.h>
}


#define BEAMER_SCALE .0002




#define SQRT2P2 (M_SQRT2/2.)

struct hardpoint_static *Assault::hardpoints = NULL/*[5] = {
	hardpoint_static(Vec3d(.000, 50 * BEAMER_SCALE, -110 * BEAMER_SCALE), Quatd(0,0,0,1), "Top Turret", 0),
	hardpoint_static(Vec3d(.000, -50 * BEAMER_SCALE, -110 * BEAMER_SCALE), Quatd(0,0,1,0), "Bottom Turret", 0),
	hardpoint_static(Vec3d(40 * BEAMER_SCALE,  .000, -225 * BEAMER_SCALE), Quatd(0,0,-SQRT2P2,SQRT2P2), "Right Turret", 0),
	hardpoint_static(Vec3d(-40 * BEAMER_SCALE,  .000, -225 * BEAMER_SCALE), Quatd(0,0,SQRT2P2,SQRT2P2), "Left Turret", 0),
	hardpoint_static(Vec3d(0, 0, 0), Quatd(0,0,0,1), "Shield Generator", 0),
}*/;
int Assault::nhardpoints = 0;



suf_t *Assault::sufbase = NULL;

Assault::Assault(WarField *aw) : st(aw){
	st::init();
	init();
	for(int i = 0; i < nhardpoints; i++){
		aw->addent(turrets[i] = (i % 2 ? new MTurret(this, &hardpoints[i]) : new GatlingTurret(this, &hardpoints[i])));
	}
}

void Assault::init(){
	if(!hardpoints){
		hardpoints = hardpoint_static::load("assault.hb", nhardpoints);
	}
	turrets = new ArmBase*[nhardpoints];
}

const char *Assault::idname()const{
	return "assault";
}

const char *Assault::classname()const{
	return "Assault";
}

const unsigned Assault::classid = registerClass("Assault", Conster<Assault>);

void Assault::serialize(SerializeContext &sc){
	st::serialize(sc);
	for(int i = 0; i < nhardpoints; i++)
		sc.o << turrets[i];
}

void Assault::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	for(int i = 0; i < nhardpoints; i++)
		sc.i >> turrets[i];
}

const char *Assault::dispname()const{
	return "Sabre class";
}

void Assault::anim(double dt){
	if(0 < health){
		inputs.press = 0;
		if(!enemy){ /* find target */
			double best = 20. * 20.;
			Entity *t;
			for(t = w->el; t; t = t->next) if(t != this && t->race != -1 && t->race != this->race && 0. < t->health && hitradius() / 2. < t->hitradius()/* && t->vft != &rstation_s*/){
				double sdist = (this->pos - t->pos).slen();
				if(sdist < best){
					this->enemy = t;
					best = sdist;
				}
			}
		}

/*		if(p->dock && p->undocktime == 0){
			if(pt->enemy)
				assault_undock(p, p->dock);
		}
		else */
		if(this->enemy){
			Vec3d xh, yh;
			long double sx, sy, len, len2, maxspeed = getManeuve().maxanglespeed * dt;
			Quatd qres, qrot;
			Vec3d dv = this->enemy->pos - this->pos;
			dv.normin();
			Vec3d forward = this->rot.trans(avec3_001);
			forward *= -1;
			xh = forward.vp(dv);
			len = len2 = xh.len();
			len = asinl(len);
			if(maxspeed < len){
				len = maxspeed;
			}
			len = sinl(len / 2.);
			if(len && len2){
				(Vec3d&)qrot = xh * (len / len2);
				qrot[3] = sqrt(1. - len * len);
				qres = qrot * this->rot;
				this->rot = qres.norm();
			}
			if(.9 < dv.sp(forward)){
				this->inputs.change |= PL_ENTER;
				this->inputs.press |= PL_ENTER;
				if(5. * 5. < (this->enemy->pos - this->pos).slen())
					this->inputs.press |= PL_W;
				else if((this->enemy->pos, this->pos).slen() < 1. * 1.)
					this->inputs.press |= PL_S;
				else
					this->inputs.press |= PL_A; /* strafe around */
			}
		}
	}
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
	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);
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
	/*glwAppend*/(new GLWarms("Arms", ppl->selected));
	return 0;
}

