/** \file
 * \brief Definition of Missile class.
 */
#ifndef MISSILE_H
#define MISSILE_H
#include "Bullet.h"
#include "tefpol3d.h"


/// \brief A homing missile.
class EXPORT Missile : public Bullet{
protected:
	Tefpol *pf;
	double integral; /* integrated term */
	double def[2]; /* integrated deflaction of each axes */
	double ft; /* fpol time */
	float fuel, throttle;
	bool centered; /* target must be centered before integration starts */
	static const double acceleration;
	static const double speedDecayRate;
	static const float maxfuel;
	void steerHoming(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void unlinkTarget();
	void initFpol();
	void updateFpol();
	static const double modelScale;
public:
	typedef Bullet st;
	Missile(Game *game) : st(game), pf(NULL), ft(0), fuel(maxfuel), throttle(0){}
	Missile(Entity *parent, float life, double damage, Entity *target = NULL);
	~Missile();
	static const unsigned classid;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double getHitRadius()const;
	virtual void enterField(WarField *);
	static const double maxspeed;

	/// \brief The type for the map object targetmap.
	///
	/// The second argument in the template parameters could be a ObservableSet<Missile>,
	/// but it costs redundant memory blocks because a Missile can have up to only one
	/// target.
	/// The embedded linked list is fast, but unfortunately not intuitive.
	typedef ObservableMap<const Entity, ObservableSet<Missile> > TargetMap;

	/// \brief Returns the global map that accumulates estimated missile damages.
	///
	/// Probably it should be a member of the object who launched the missiles.
	/// The launcher avoids to waste missiles on too small targets with this map.
	static TargetMap &targetmap();
};


#endif
