/** \file
 * \brief Definition of BeamProjectile class.
 */
#include "BeamProjectile.h"
#include "Viewer.h"
#include "serial_util.h"
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

const unsigned BeamProjectile::classid = registerClass("BeamProjectile", Conster<BeamProjectile>);

BeamProjectile::BeamProjectile(Game *game) : st(game), cs(&cs_beamtrail), pf(NULL){}

BeamProjectile::BeamProjectile(Entity *owner, float life, double damage, double radius, Vec4<unsigned char> col, const color_sequence &cs, double getHitRadius)
: st(owner, life, damage), pf(NULL), radius(radius), col(col), cs(&cs), m_hitradius(getHitRadius){
}

const char *BeamProjectile::classname()const{
	return "BeamProjectile";
}

template<> SerializeStream &SerializeStream::operator<<(const Vec4<unsigned char> &v){
	return *this << (v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24));
}

template<> UnserializeStream &operator>>(UnserializeStream &o, Vec4<unsigned char> &v){
	unsigned long col;
	o >> col;
	v = Vec4<unsigned char>(col & 0xff, (col >> 8) & 0xff, (col >> 16) & 0xff, col >> 24);
	return o;
}

void BeamProjectile::serialize(SerializeContext &sc){
	st::serialize(sc);
	sc.o << radius;
	sc.o << col;
}

void BeamProjectile::unserialize(UnserializeContext &sc){
	st::unserialize(sc);
	sc.i >> radius;
	sc.i >> col;
}

void BeamProjectile::enterField(WarField *w){
#ifndef DEDICATED
	if(WarSpace *ws = *w)
		pf = ws->tepl->addTefpolMovable(pos, velo, avec3_000, cs, TEP3_THICKER, cs->t);
	else
		pf = NULL;
#endif
}

void BeamProjectile::leaveField(WarField *w){
#ifndef DEDICATED
	if(pf){
		pf->immobilize();
		pf = NULL;
	}
#endif
}

void BeamProjectile::anim(double dt){
	st::anim(dt);
}

void BeamProjectile::clientUpdate(double dt){
#ifndef DEDICATED
	st::clientUpdate(dt);
	if(this->pf){
		this->pf->move(pos, avec3_000, cs->t, 0);
	}
#endif
}

#ifdef DEDICATED
void BeamProjectile::drawtra(WarDraw *){}
#else
void BeamProjectile::drawtra(wardraw_t *wd){
//	const GLubyte acol[4] = {COLOR32R(col), COLOR32G(col), COLOR32B(col), COLOR32A(col)};
	gldSpriteGlow(pos, radius, col, wd->vw->irot);
}
#endif

double BeamProjectile::getHitRadius()const{return m_hitradius;}
