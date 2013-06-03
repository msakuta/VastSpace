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
#include "ModelEntity.h"

struct Model;

/// \brief Base class for aerial vehicles.
class Aerial : public ModelEntity{
public:
	typedef ModelEntity st;
	Aerial(Game *game);
	Aerial(WarField *aw);
	void addRigidBody(WarSpace*)override;
	void control(const input_t *inputs, double dt)override;
	void anim(double dt)override;
	Props props()const override;
	bool isTargettable()const override{return true;}
	bool isSelectable()const override{return true;}
	void start_control();
	void end_control();

	static gltestp::dstring modPath(){return "surface/";}

protected:
	double aileron, elevator, rudder;
	double throttle;
	double gearphase;
	double cooldown;
	std::vector<Vec3d> force;
	Vec3d sight;
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
		enum class Control{None, Aileron, Elevator, Rudder} control; ///< Control surface definition.
		Vec3d axis; ///< The aerodynamic tensor is rotated around this axis if this control surface is manipulated.
		double sensitivity; ///< Sensitivity of this control surface when this surface is manipulated.

		Wing() : control(Control::None){}
	};

	typedef std::vector<Wing> WingList;

	class WingProcess;
	class Vec3dListProcess;

	virtual WingList &getWings()const = 0;
	virtual HitBoxList &getHitBoxes()const = 0;
	virtual bool buildBody();
	virtual short bbodyGroup()const;
	virtual short bbodyMask()const;
	virtual void shoot(double dt);
	virtual double getThrustStrength()const = 0;

	void init();
	bool cull(WarDraw *)const;
};

/// \brief Processes a WingList value in a Squirrel script.
class Aerial::WingProcess : public SqInitProcess{
public:
	WingList &value;
	const SQChar *name;
	bool mandatory;
	WingProcess(WingList &value, const SQChar *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	void process(HSQUIRRELVM)const override;
};

/// \brief Processes a list of Vec3d in a Squirrel script.
class Aerial::Vec3dListProcess : public SqInitProcess{
public:
	std::vector<Vec3d> &value;
	const SQChar *name;
	bool mandatory;
	Vec3dListProcess(std::vector<Vec3d> &value, const SQChar *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	void process(HSQUIRRELVM)const override;
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

protected:
	Tefpol *pf;
	double ttl;
};

extern const struct color_sequence cs_firetrail;

#endif