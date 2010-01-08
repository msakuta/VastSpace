#ifndef SHIELD_H
#define SHIELD_H
// shield effect
#include "entity.h"

class Bullet;

class ShieldEffect{
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
};

inline ShieldEffect::ShieldEffect() : p(NULL), acty(0){}

#endif
