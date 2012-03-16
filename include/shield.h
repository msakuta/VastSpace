#ifndef SHIELD_H
#define SHIELD_H
/** \file
 * \brief Definition of ShieldEffect class.
 */
// shield effect
#include "Entity.h"

class Bullet;

/// \breif The class that manages shield effects.
///
/// It's only functional in Windows client. In dedicated server,
/// invocations of member functions are just ignored.
class EXPORT ShieldEffect{
#ifdef DEDICATED
public:
	void bullethit(const Entity *, const Bullet *, double){}
	void takedamage(double){}
	void anim(double){}
	void draw(wardraw_t *, const Entity *, double, double){}
#else
public:
	ShieldEffect();
	~ShieldEffect();
	void bullethit(const Entity *pe, const Bullet *, double maxshield);
	void takedamage(double rdamage);
	void anim(double dt);
	void draw(wardraw_t *wd, const Entity *base, double rad, double ratio);
private:
	class Wavelet;
	class WaveletList;
	double acty; // Activity of the shield effect, reacts to interference
	Wavelet *p; // List of wavelets invoked
#endif
};

#ifndef DEDICATED
inline ShieldEffect::ShieldEffect() : p(NULL), acty(0){}
#endif

#endif
