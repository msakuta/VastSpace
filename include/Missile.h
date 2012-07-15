/** \brief Definition of Missile class.
 */
#ifndef MISSILE_H
#define MISSILE_H
#include "Bullet.h"
#include "tent3d.h"


/// \brief A homing missile.
class EXPORT Missile : public Bullet{
	struct tent3d_fpol *pf;
	double integral; /* integrated term */
	double def[2]; /* integrated deflaction of each axes */
	double ft; /* fpol time */
	float fuel, throttle;
	bool centered; /* target must be centered before integration starts */
	static const float maxfuel;
	void steerHoming(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void unlinkTarget();
public:
	typedef Bullet st;
	Missile(Game *game) : st(game), pf(NULL), ft(0), fuel(maxfuel), throttle(0){}
	Missile(Entity *parent, float life, double damage, Entity *target = NULL);
	~Missile();
	static const unsigned classid;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double hitradius()const;
	static const double maxspeed;

	/// \brief The event notified when a Missile in a damage accumulation chain gets deleted.
	struct UnlinkEvent : public ObserveEvent{
		static ObserveEventID sid;
		ObserveEventID id()const{return sid;}
		UnlinkEvent(Missile *next) : next(next){}
		Missile *next;
	};

	/// \brief The list node pointing to the next node.
	///
	/// If the pointed node (next element) gets deleted, this pointer will automatically
	/// become to point to the next element's next element.
	class MissileList : public WeakPtr<Missile>{
	public:
		MissileList(Missile *m = NULL) : WeakPtr<Missile>(m){}
		MissileList &operator=(Missile *m){
			WeakPtr<Missile>::operator =(m);
			return *this;
		}
		/// We use UnlinkEvent to replace the next pointer rather than to override unlink().
		/// That's because when unlink() is called, pointed object (Missile) is already reduced to
		/// Observable, so we cannot assure Missile class member variable is still valid.
		/// We could downcast the Observable pointer to Missile pointer, but it's conceptually
		/// undesirable for OO mind.
		virtual bool handleEvent(const Observable *o, ObserveEvent &e){
			if(UnlinkEvent *ue = InterpretEvent<UnlinkEvent>(e)){
				ptr->removeObserver(this);
				ptr = ue->next;
				if(ptr)
					ptr->addObserver(this);
				return true;
			}
			else
				return WeakPtr<Missile>::handleEvent(o, e);
		}
	};

	MissileList targetlist;
//	static std::map<const Entity *, Missile *> targetmap;
	typedef ObservableMap<const Entity, MissileList > TargetMap;

	/// \brief The global map that accumulates estimated missile damages.
	///
	/// Probably it should be a member of the object who launched the missiles.
	/// The launcher avoids to waste missiles on too small targets with this map.
	static TargetMap targetmap;
};


#endif
