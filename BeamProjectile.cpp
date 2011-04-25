/** \file
 * \brief Definition of BeamProjectile class.
 */
#include "BeamProjectile.h"
#include "Viewer.h"
#include "draw/WarDraw.h"
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
const struct color_sequence BeamProjectile::cs_beamtrail = DEFINE_COLSEQ(cnl_beamtrail, (COLOR32)-1, 1.);

BeamProjectile::BeamProjectile(Entity *owner, float life, double damage, double radius, Vec4<unsigned char> col, const color_sequence &cs)
: st(owner, life, damage), pf(NULL), radius(radius), col(col), cs(&cs){
}

void BeamProjectile::enterField(WarField *w){
	if(WarSpace *ws = *w)
		pf = AddTefpolMovable3D(ws->tepl, pos, velo, avec3_000, cs, TEP3_THICKER, cs->t);
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
		MoveTefpol3D(this->pf, pos, avec3_000, cs->t, 0);
	}
	st::anim(dt);
}

void BeamProjectile::drawtra(wardraw_t *wd){
//	const GLubyte acol[4] = {COLOR32R(col), COLOR32G(col), COLOR32B(col), COLOR32A(col)};
	gldSpriteGlow(pos, radius, col, wd->vw->irot);
}

double BeamProjectile::hitradius()const{return .01;}
