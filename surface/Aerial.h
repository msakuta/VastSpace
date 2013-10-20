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
#include "SqInitProcess-ex.h"

#include <btBulletDynamicsCommon.h>

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

	double getRudder()const;

	static gltestp::dstring modPath(){return "surface/";}

protected:
	double aileron, elevator, rudder;
	double fspoiler;
	double throttle;
	double gearphase;
	double cooldown;
	double iaileron; ///< Integrated aileron
	std::vector<Vec3d> force;
	Vec3d sight;
	bool muzzle, brake, afterburner, navlight, gear;
	bool spoiler;
	bool showILS;
	int missiles;
	int weapon;
//	sufdecal_t *sd;
//	bhole_t *frei;
//bhole_t bholes[50];
	Vec3d destPos; ///< Destination position
	bool destArrived; ///< If arrived to the destination
	bool onFeet; ///< If landed on ground
	bool takingOff; ///< If this plane's auto pilot is taking off
	double takeOffTimer;
	WeakPtr<Bullet> lastMissile; ///< Reference to last shot missile
	WeakPtr<Entity> landingAirport; ///< The airport this airplane is going to land with autopilot.

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

	struct CameraPos{
		Vec3d pos;
		Quatd rot;
		enum class Type{Normal, Cockpit, MissileTrack, ViewTrack, Rotate} type;
	};

	typedef std::vector<CameraPos> CameraPosList;

	class CameraPosProcess;

	SQInteger sqGet(HSQUIRRELVM v, const SQChar *name)const override;
	SQInteger sqSet(HSQUIRRELVM v, const SQChar *name)override;
	virtual WingList &getWings()const = 0;
	virtual HitBoxList &getHitBoxes()const = 0;
	virtual bool buildBody();
	virtual short bbodyGroup()const;
	virtual short bbodyMask()const;
	virtual void shoot(double dt);
	virtual double getThrustStrength()const = 0;
	virtual void toggleWeapon(){weapon = !weapon;}
	virtual btCompoundShape *getShape() = 0;

	void init();
	bool cull(WarDraw *)const;
	bool taxi(double dt);
	void animAI(double dt, bool onfeet);
	void calcCockpitView(Vec3d &pos, Quatd &rot, const CameraPos &cam)const;
	btCompoundShape *buildShape();
	void drawDebugWings()const;
	void drawCockpitHUD(const Vec3d &hudPos, double hudSize, const Vec3d &seat,
		const Vec3d &gunDirection, const StringList &weaponList)const;
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

/// \brief Processes a WingList value in a Squirrel script.
class Aerial::CameraPosProcess : public SqInitProcess{
public:
	CameraPosList &value;
	const SQChar *name;
	bool mandatory;
	CameraPosProcess(CameraPosList &value, const SQChar *name = _SC("cameraPositions"), bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
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
