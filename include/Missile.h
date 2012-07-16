/** \file
 * \brief Definition of Missile class.
 */
#ifndef MISSILE_H
#define MISSILE_H
#include "Bullet.h"
#include "tent3d.h"


/// \brief A homing missile.
class EXPORT Missile : public Bullet{
protected:
	struct tent3d_fpol *pf;
	double integral; /* integrated term */
	double def[2]; /* integrated deflaction of each axes */
	double ft; /* fpol time */
	float fuel, throttle;
	bool centered; /* target must be centered before integration starts */
	static const float maxfuel;
	void steerHoming(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void unlinkTarget();
	void initFpol();
	void updateFpol();
public:
	typedef Bullet st;
	Missile(Game *game) : st(game), pf(NULL), ft(0), fuel(maxfuel), throttle(0), targetnext(NULL), targetprev(NULL){}
	Missile(Entity *parent, float life, double damage, Entity *target = NULL);
	~Missile();
	static const unsigned classid;
	virtual const char *classname()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void postframe();
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual double hitradius()const;
	virtual void enterField(WarField *);
	static const double maxspeed;

	/// \brief The head node in the linked list for a targetted object.
	///
	/// It is merely a pointer to Missile object, but has a destructor that automatically 
	/// clear the reference from the pointed node.
	class MissileList{
	public:
		Missile *ptr;
		MissileList(Missile *a = NULL) : ptr(a){}
		~MissileList(){
			if(ptr)
				ptr->targetprev = NULL;
		}
		MissileList &operator=(Missile *a){
			ptr = a;
			return *this;
		}
	};

	/// \brief The pointer to the next node in the Missile list.
	///
	/// The linked list is bi-directional, i.e. each node has previous and next pointers.
	/// The next (forward) pointer is used to trace the chain.
	Missile *targetnext;

	/// \brief The pointer to pointer that points back to us in the previous node.
	///
	/// It is used when this node is being deleted, to clear or replace the previous
	/// pointer so that the whole list keeps containing the whole nodes but this one.
	///
	/// The previous node is not necessarily a Missile object but also can be head node
	/// (MissileList object), but we do not want to treat the cases individually, so
	/// it has a type of pointer to pointer.
	Missile **targetprev;

	/// \brief The type for the map object targetmap.
	///
	/// The second argument in the template parameters could be a ObservableSet<Missile>,
	/// but it costs redundant memory blocks because a Missile can have up to only one
	/// target.
	/// The embedded linked list is fast, but unfortunately not intuitive.
	typedef ObservableMap<const Entity, MissileList > TargetMap;

	/// \brief The global map that accumulates estimated missile damages.
	///
	/// Probably it should be a member of the object who launched the missiles.
	/// The launcher avoids to waste missiles on too small targets with this map.
	static TargetMap targetmap;
};


#endif
