/** \file
 * \brief Definition of Autonomous class.
 */
#ifndef AUTONOMOUS_H
#define AUTONOMOUS_H

#include "Entity.h"
#include "SqInitProcess.h"
#include "Game.h"
#include "judge.h"
#ifndef DEDICATED
#ifdef _WIN32
#include <windows.h>
#endif
#include <gl/GL.h>
#else
typedef unsigned int GLuint;
#endif


struct hitbox;
class EXPORT HitBoxList : public std::vector<hitbox>{
};
struct hardpoint_static;

/* List of spaceship tasks that all types of ship share. */
enum sship_task{
	sship_idle, /* Approach to stationary orbit; relative velocity zero against local coordsys. */
	sship_undock, /* Undocked and exiting */
	sship_undockque, /* Queue in a line of undock waiting. */
	sship_dock, /* Docking */
	sship_dockque, /* Queue in a line of dock waiting. */
	sship_moveto, /* Move to indicated location. */
	sship_parade, /* Parade formation. */
	sship_delta, // Delta formation
	sship_attack, /* Attack to some object. */
	sship_away, /* Fighters going away in attacking cycle. */
	sship_gather, /* Gather resources. */
	sship_harvest, /* Harvest resources. */
	sship_harvestreturn, /* Harvest resources. */
	sship_warp, // Warping
	num_sship_task
};


/// \brief An autonomous object, which means it can move around by itself, but not remote control like Missiles.
class EXPORT Autonomous : public Entity{
public:
	typedef Entity st;
	Vec3d dest; // Move order destination
	enum sship_task task;
	int direction;

	Autonomous(Game *game) : st(game){}
	Autonomous(WarField *w);

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void addRigidBody(WarSpace*);
	virtual void anim(double dt);
	virtual void drawtra(wardraw_t *);
	virtual void drawHUD(wardraw_t *);
	virtual void control(const input_t *, double);
	unsigned analog_mask();
	virtual Props props()const;
	struct ManeuverParams;
	virtual const ManeuverParams &getManeuve()const;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual bool command(EntityCommand *com);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);

	virtual short getDefaultCollisionMask()const;

	/// \brief Returns velocity vector in absolute coordinates.
	///
	/// In the relative-space, there's no such thing like an absolute velocity, but it's used to measure
	/// inter-CoordSys movements, such as warping.
	virtual Vec3d absvelo()const;

	void maneuver(const Mat4d &mat, double dt, const ManeuverParams *mn);
	void steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale);

	/// \brief A set of fields that defines maneuverability of the ship.
	struct ManeuverParams{
		double accel; ///< Linear acceleration rate [m/s^2]
		double maxspeed; ///< Maximum linear speed [m/s], which wouldn't really exist in space, but provided for safe sailing.
		double angleaccel; ///< Angular acceleration [rad/s^2]
		double maxanglespeed; ///< Maximum angular speed [rad/s], which also wouldn't exist.
		double capacity; ///< The capacity of capacitor [MJ]
		double capacitor_gen; ///< Rate of generated energy [MW]
	};
protected:
	virtual void init();
	virtual bool buildBody();
	virtual short bbodyGroup()const;
	virtual short bbodyMask()const;
	virtual std::vector<hitbox> *getTraceHitBoxes()const;

	/// \brief Adapter function that only transmits arguments to SqInit().
	bool sq_init(const SQChar *scriptFile, const SqInitProcess &procs){
		return SqInit(game->sqvm, scriptFile, procs);
	}

	class EXPORT ModelScaleProcess : public SingleDoubleProcess{
	public:
		ModelScaleProcess(double &modelScale) : SingleDoubleProcess(modelScale, "modelScale"){}
	};

	class EXPORT MassProcess : public SingleDoubleProcess{
	public:
		MassProcess(double &mass) : SingleDoubleProcess(mass, "mass"){}
	};

	/// \brief Processes single Vec3d with given variable name.
	class EXPORT Vec3dProcess : public SqInitProcess{
	public:
		Vec3d &vec;
		const SQChar *name;
		bool mandatory;
		Vec3dProcess(Vec3d &vec, const SQChar *name, bool mandatory = true) : vec(vec), name(name), mandatory(mandatory){}
		virtual void process(HSQUIRRELVM)const;
	};

	/// \brief A class that processes maneuver parameters.
	class EXPORT ManeuverParamsProcess : public SqInitProcess{
	public:
		ManeuverParams &mn;
		ManeuverParamsProcess(ManeuverParams &mn) : mn(mn){}
		virtual void process(HSQUIRRELVM)const;
	};

	struct EnginePos{ Vec3d pos; Quatd rot; };
	typedef std::vector<EnginePos> EnginePosList;
	class EXPORT EnginePosListProcess : public SqInitProcess{
	public:
		EnginePosList &vec;
		const SQChar *name;
		bool mandatory;
		EnginePosListProcess(EnginePosList &vec, const SQChar *name, bool mandatory = true) : vec(vec), name(name), mandatory(mandatory){}
		virtual void process(HSQUIRRELVM)const;
	};

	class EXPORT HitboxProcess : public SqInitProcess{
	public:
		HitBoxList &hitboxes;
		HitboxProcess(HitBoxList &hitboxes) : hitboxes(hitboxes){}
		virtual void process(HSQUIRRELVM)const;
	};

	class EXPORT HardPointProcess : public SqInitProcess{
	public:
		std::vector<hardpoint_static*> &hardpoints;
		HardPointProcess(std::vector<hardpoint_static*> &hardpoints) : hardpoints(hardpoints){}
		virtual void process(HSQUIRRELVM)const;
	};

public: // Publish the process object temporarily for the Entities that do not inherit Autonomous.
	class EXPORT DrawOverlayProcess : public SqInitProcess{
	public:
		GLuint &disp;
		DrawOverlayProcess(GLuint &disp) : disp(disp){}
		virtual void process(HSQUIRRELVM v)const;
	};
protected:

	/// \brief Static information for navigation lights.
	struct Navlight{
		Vec3d pos; ///< Position of this navlight relative to origin of the Entity.
		Vec4f color; ///< Color values in RGBA
		float radius; ///< Apparent radius of the light.
		float period; ///< Period of light cycle
		float phase; ///< Phase offset in the period
		enum Pattern{Constant, Triangle, Step} pattern; ///< Pattern of light intensity over time
		float duty; ///< Duty factor of pattern in case of Step.

		/// \brief The constructor. Try to keep it a POD.
		Navlight();

		/// \brief Returns pattern's intensity at given time.
		/// \param t Time parameter in the range of [0,period)
		double patternIntensity(double t)const;
	};

	class EXPORT NavlightsProcess : public SqInitProcess{
	public:
		std::vector<Navlight> &navlights;
		NavlightsProcess(std::vector<Navlight> &navlights) : navlights(navlights){}
		virtual void process(HSQUIRRELVM v)const;
	};

	/// \brief Helper function to draw navigation lights.
	/// \param wd The WarDraw object.
	/// \param navlights List of navigation lights.
	/// \param transmat The transformation for the lights. Can be NULL to specify
	///        the default transformation, i.e. return value of transform().
	void drawNavlights(WarDraw *wd, const std::vector<Navlight> &navlights, const Mat4d *transmat = NULL);
private:
	static const ManeuverParams mymn;
};

EXPORT void draw_healthbar(Entity *pt, wardraw_t *wd, double v, double scale, double s, double g);
#ifdef NDEBUG
#define hitbox_draw
#else
void hitbox_draw(const Entity *pt, const double sc[3], int hitflags = 0);
#endif

void space_collide(Entity *pt, WarSpace *w, double dt, Entity *collideignore, Entity *collideignore2);



//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

inline Autonomous::Navlight::Navlight() : pos(0,0,0), color(1,0,0,1), radius(0.01f), period(1.), phase(0.), pattern(Triangle), duty(0.1){
}


#endif
