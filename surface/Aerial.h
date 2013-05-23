/** \file
 * \brief Definition of base classes for aerial Entities such as aeroplanes.
 *
 * This header and its corresponding sources (Aerial.cpp and Aerial-draw.cpp) will use
 * experimentally C++11's new features excessively.
 * Thus, VC2008 shouldn't be able to compile them.
 */
#ifndef AERIAL_H
#define AERIAL_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include "Entity.h"

struct Model;

/// \brief Base class for aerial vehicles.
class Aerial : public Entity{
public:
	typedef Entity st;
	static EntityRegister<Aerial> entityRegister;
	Aerial(Game *game);
	Aerial(WarField *aw);
	void drawHUD(WarDraw *);
	void cockpitView(Vec3d &pos, Quatd &rot, int *seatid);
	void control(input_t *inputs, double dt);
	void anim(double dt);
	void draw(WarDraw *wd);
	void drawtra(WarDraw *wd);
	int takedamage(double damage, int hitpart);
	void drawCockpit(WarDraw *);
	const char *idname()const{return "fly";}
	const char *classname()const override{return "Fly";}
	void start_control();
	void end_control();
	double getHitRadius()const override{return 0.020;}
	bool isTargettable()const override{return true;}
	double getMaxHealth()const override{return maxHealthValue;}

	static gltestp::dstring modPath(){return "surface/";}
	static double getModelScale(){return modelScale;}

protected:
	double aileron[2], elevator, rudder;
	double throttle;
	double gearphase;
	double cooldown;
	Vec3d force[5], sight;
	Tefpol *pf, *vapor[2];
	char muzzle, brk, afterburner, navlight, gear;
	int missiles;
	int weapon;
//	sufdecal_t *sd;
//	bhole_t *frei;
//bhole_t bholes[50];

	/// \brief An internal structure that representing a wing and its parameters.
	struct Wing{
		Vec3d pos; ///< Position of the wing's center, relative to center of mass
		Mat3d aero; ///< The aerodynamic tensor, defines how force is applied to the wing.
		gltestp::dstring name; ///< Name of the wing, just for debugging
	};

	typedef std::vector<Wing> WingList;

	class WingProcess;

	static Model *model;
	static double modelScale;
	static double defaultMass; ///< Dry mass?
	static double maxHealthValue;
	static WingList wings0;

	void init();
	void shootDualGun(double dt);
	bool cull(WarDraw *)const;
};


/// \brief A light-emitting flare dropped from an Aerial object.
class Flare : public Entity{
public:
	typedef Entity st;
	Flare(Game *);
	Flare(WarField *);
	void anim(double dt);
	void drawtra(WarDraw *wd);
//	int flare_flying(const entity_t *pt);

	/*
struct entity_private_static flare_s = {
	{
		NULL,
		NULL,
	},
	flare_anim,
	NULL,
	flare_drawtra,
	NULL,
	NULL,
	NULL,
	flare_flying,
	0., 0., 0.,
	NULL, NULL, NULL, NULL,
	0,
	0.
};
*/
	Tefpol *pf;
	double ttl;
};

#endif
