/** \file
 * \brief Definition of Launcher class and its companions.
 */
#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "arms.h"
#include "Bullet.h"

/// \brief Base class for rocket or missile launchers.
class Launcher : public ArmBase{
public:
	typedef ArmBase st;
	Launcher(Game *game) : st(game), cooldown(0.), ammo(0){}
	Launcher(Entity *base, const hardpoint_static *hp, int ammo_ = 4) : st(base, hp), cooldown(0.), ammo(ammo_){}

	static gltestp::dstring modPath(){return "surface/";}
protected:
	SQInteger sqGet(HSQUIRRELVM, const SQChar *)const override;
	SQInteger sqSet(HSQUIRRELVM, const SQChar *)override;

	double cooldown;
	int ammo;
};

/// \brief Hydra-70 unguided rocket.
class HydraRocket : public Bullet{
public:
	typedef Bullet st;
	static EntityRegister<HydraRocket> entityRegister;
	HydraRocket(Game *game) : st(game), pf(NULL), fuel(3.){}
	HydraRocket(WarField *w);
	HydraRocket(Entity *owner, float life, double damage) :
		st(owner, life, damage), pf(NULL), fuel(3.){}
	const char *classname()const override{return "HydraRocket";}
	void anim(double dt)override;
	void clientUpdate(double dt)override;
	void draw(WarDraw *wd)override;
	void drawtra(WarDraw *wd)override;
	void enterField(WarField*)override;
	void leaveField(WarField*)override;

protected:
	static const double modelScale;

	Tefpol *pf;
	double fuel;

	void commonUpdate(double dt);
	void bulletDeathEffect(int hitground, const struct contact_info *ci)override;
};

/// \brief M261, a Hydra-70 rocket launcher mounted on attack helicopters.
class HydraRocketLauncher : public Launcher{
public:
	typedef Launcher st;
	HydraRocketLauncher(Game *game) : st(game){}
	HydraRocketLauncher(Entity *base, const hardpoint_static *hp) : st(base, hp, 19){}
	const char *classname()const override{return "HydraRocketLauncher";}
	void anim(double dt)override;
	void draw(WarDraw *)override;
};

/// \brief AGM-114 Hellfire air-to-surface missile.
///
/// http://en.wikipedia.org/wiki/AGM-114_Hellfire
class Hellfire : public Bullet{
public:
	typedef Bullet st;
	static EntityRegister<Hellfire> entityRegister;
	Hellfire(Game *game) : st(game), pf(NULL), fuel(3.), target(NULL), centered(false){def[0] = def[1] = 0.;}
	Hellfire(WarField *w);
	Hellfire(Entity *owner, float life, double damage) :
		st(owner, life, damage), pf(NULL), fuel(3.), target(NULL), centered(false){def[0] = def[1] = 0.;}
	const char *classname()const override{return "Hellfire";}
	void anim(double dt)override;
	void clientUpdate(double dt)override;
	void draw(WarDraw *wd)override;
	void drawtra(WarDraw *wd)override;
	void enterField(WarField*)override;
	void leaveField(WarField*)override;

protected:
	SQInteger sqGet(HSQUIRRELVM, const SQChar *)const override;
	SQInteger sqSet(HSQUIRRELVM, const SQChar *)override;

	static const double modelScale;

	Tefpol *pf;
	double fuel;
	WeakPtr<Entity> target;
	double integral; ///< integrated term
	double def[2]; ///< integrated deflection of each axes
	bool centered; ///< target must be centered before integration starts

	void commonUpdate(double dt);
	void bulletDeathEffect(int hitground, const struct contact_info *ci)override;

	friend class HellfireLauncher;
};

/// \brief Hellfire launcher mounted on attack helicopters.
class HellfireLauncher : public Launcher{
public:
	typedef Launcher st;
	HellfireLauncher(Game *game) : st(game){}
	HellfireLauncher(Entity *base, const hardpoint_static *hp) : st(base, hp, 4){}
	const char *classname()const override{return "HellfireLauncher";}
	void anim(double dt)override;
	void draw(WarDraw *)override;
};



#endif
