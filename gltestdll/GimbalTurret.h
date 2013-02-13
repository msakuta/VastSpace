/** \file
 * \brief Definition of GimbalTurret and MissileGimbalTurret classes.
 */
#ifndef GIMBALTURRET_H
#define GIMBALTURRET_H

#include "Autonomous.h"

struct Model;
struct Motion;

/// \brief A stationary (in a linear velocity sense) turret in space that can face any direction quickly.
///
class GimbalTurret : public Autonomous{
public:
	typedef Autonomous st;
protected:
public:
	GimbalTurret(Game *game) : st(game){init();}
	GimbalTurret(WarField *w);
	~GimbalTurret();
	void init();
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<GimbalTurret> entityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual const char *dispname()const;
	virtual void anim(double);
	virtual void clientUpdate(double);
	virtual void postframe();
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void enterField(WarField *);
	virtual void leaveField(WarField *);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual double maxhealth()const;
	virtual double getHitRadius()const{return hitRadius;}
	virtual bool isTargettable()const{return true;}
	virtual bool isSelectable()const{return true;}
	virtual bool dock(Docker*);
	virtual bool undock(Docker*);
	static Entity *create(WarField *w, Builder *);

	/// \brief Retrieves root path for this extension module.
	static gltestp::dstring modPath(){return "gltestdll/";}

protected:
	bool buildBody();
	bool initModel();
	virtual float reloadtime()const; ///< Overridable getter that returns interval time between shoots
	virtual double bulletspeed()const; ///< Overridable getter that returns shot bullet's speed.
	virtual float bulletlife()const; ///< Overridable getter that returns time to live for shot bullets in seconds.
	virtual double bulletDamage()const; ///< Overridable getter that returns shot bullet's damage.
	virtual Bullet *createBullet(const Vec3d &gunPos); ///< Overridable method to create shot bullet.
	void findtarget(const Entity *ignore_list[], int nignore_list);
	double findtargetproc(const Entity *target)const;
	void shoot(double dt);
	void deathEffect();

	double yaw;
	double pitch;
	double cooldown;
	float muzzleFlash;
	bool deathEffectDone;

	static Model *model;
	static Motion *motions[2];
	static double modelScale;
	static double defaultMass;
	static double hitRadius;
	static double maxHealthValue;
	static GLuint overlayDisp;
	static Vec3d gunPos[2];
};


/// \brief GimbalTurret that shoots missiles.
class MissileGimbalTurret : public GimbalTurret{
public:
	typedef GimbalTurret st;
	MissileGimbalTurret(Game *game) : st(game){}
	MissileGimbalTurret(WarField *w) : st(w){}
	const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<MissileGimbalTurret> entityRegister;
protected:
	float reloadtime()const{return 2.;}
	double bulletspeed()const{return 0.4;}
	float bulletlife()const{return 10.;}
	double bulletDamage()const{return 100.;}
	Bullet *createBullet(const Vec3d &gunPos);
};

#endif
