#include "beamer.h"
#include "judge.h"
extern "C"{
#include <clib/gl/gldraw.h>
}


#define BEAMER_SCALE .0002



suf_t *Assault::sufbase = NULL;

Assault::Assault(){
	init();
}

const char *Assault::idname()const{
	return "assault";
}

const char *Assault::classname()const{
	return "Sabre class";
}

void Assault::anim(double dt){
	st::anim(dt);
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
		for(int i = 0; i < Beamer::beamer_nhb; i++){
			Mat4d rot;
			glPushMatrix();
			gldTranslate3dv(Beamer::beamer_hb[i].org);
			rot = Beamer::beamer_hb[i].rot.tomat4();
			glMultMatrixd(rot);
			hitbox_draw(this, Beamer::beamer_hb[i].sc);
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
