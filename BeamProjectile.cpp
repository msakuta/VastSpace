/** \file
 * \brief Definition of BeamProjectile class.
 */
#include "BeamProjectile.h"
#include "viewer.h"
extern "C"{
#include <clib/c.h>
#include <clib/gl/gldraw.h>
}

/* color sequences */
#define DEFINE_COLSEQ(cnl,colrand,life) {COLOR32RGBA(0,0,0,0),numof(cnl),(cnl),(colrand),(life),1}
static const struct color_node cnl_beamtrail[] = {
	{0.1, COLOR32RGBA(127,127,255,255)},
	{0.15, COLOR32RGBA(95,127,255,255)},
	{0.45, COLOR32RGBA(63,63,255,191)},
	{0.3, COLOR32RGBA(0,0,255,0)},
};
static const struct color_sequence cs_beamtrail = DEFINE_COLSEQ(cnl_beamtrail, (COLOR32)-1, 1.);

BeamProjectile::BeamProjectile(Entity *owner, float life, double damage)
: st(owner, life, damage), pf(NULL){
}

void BeamProjectile::enterField(WarField *w){
	if(WarSpace *ws = *w)
		pf = AddTefpolMovable3D(ws->tepl, pos, velo, avec3_000, &cs_beamtrail, TEP3_THICKER, cs_beamtrail.t);
	else
		pf = NULL;
}

void BeamProjectile::leaveField(WarField *w){
	if(pf){
		ImmobilizeTefpol3D(pf);
		pf = NULL;
	}
}

void BeamProjectile::anim(double dt){
	if(this->pf){
		MoveTefpol3D(this->pf, pos, avec3_000, cs_beamtrail.t, 0);
	}
	st::anim(dt);
}

void BeamProjectile::drawtra(wardraw_t *wd){
	const GLubyte col[4] = {127,127,255,255};
	gldSpriteGlow(pos, .01, col, wd->vw->irot);
}

double BeamProjectile::hitradius()const{return .01;}
