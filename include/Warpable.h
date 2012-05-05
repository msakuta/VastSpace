#ifndef WARPABLE_H
#define WARPABLE_H

#include "Entity.h"
#include "CoordSys.h"
#include "war.h"
#include "shield.h"
#ifndef DEDICATED
#include <gl/GL.h>
#else
typedef unsigned int GLuint;
#endif


struct hitbox;
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


class EXPORT Warpable : public Entity{
public:
	typedef Entity st;
	Vec3d dest; // Move order destination
	Vec3d warpdst;
	double warpSpeed;
	double totalWarpDist, currentWarpDist;
	double capacitor; /* Temporarily stored energy, MegaJoules */
	int warping;
	WeakPtr<CoordSys> warpcs;
	WeakPtr<CoordSys> warpdstcs;
//	WarField *warp_next_warf;
	enum sship_task task;
	int direction;

	Warpable(Game *game) : st(game){}
	Warpable(WarField *w);

	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void anim(double dt);
	virtual void drawtra(wardraw_t *);
	virtual void drawHUD(wardraw_t *);
	void control(input_t *, double);
	unsigned analog_mask();
	virtual int popupMenu(PopupMenu &list);
	virtual Warpable *toWarpable();
	virtual Props props()const;
	struct maneuve;
	virtual const maneuve &getManeuve()const;
	virtual double maxenergy()const = 0;
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual bool command(EntityCommand *com);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void post_warp();

	virtual short getDefaultCollisionMask()const;

	void maneuver(const Mat4d &mat, double dt, const struct maneuve *mn);
	void steerArrival(double dt, const Vec3d &atarget, const Vec3d &targetvelo, double speedfactor, double minspeed);
	void warp_collapse();
	void drawCapitalBlast(wardraw_t *wd, const Vec3d &nozzlepos, double scale);

	struct maneuve{
		double accel;
		double maxspeed;
		double angleaccel;
		double maxanglespeed;
		double capacity; /* capacity of capacitor [MJ] */
		double capacitor_gen; /* generated energy [MW] */
	};
protected:
	virtual void init();

	/// \brief Callback functionoid that processes Squirrel VM that is initialized in sq_init().
	///
	/// Multiple SqInitProcess derived class objects can be specified.
	///
	/// sq_init() uses temporary table that delegates the root table (can read from the root table but
	/// writing won't affect it) that will be popped from the stack before the function returns.
	/// It means the SqInitProcess derived class objects must process the defined values in the table
	/// before sq_init() returns.
	class SqInitProcess{
	public:
		SqInitProcess() : next(NULL){}

		/// \brief The method called after the VM is initialized.
		virtual void process(HSQUIRRELVM v)const = 0;

		/// \brief Multiple SqInitProcesses can be connected with this operator before passing to sq_init().
		const SqInitProcess &operator<<=(const SqInitProcess &o)const;

		const SqInitProcess &chain(const SqInitProcess &o)const;

	protected:
		mutable const SqInitProcess *next;
		friend class Warpable;
	};

	/// \brief Initializes various settings of Warpable-derived class by Squirrel script.
	bool sq_init(const SQChar *scriptFile, const SqInitProcess &procs);

	class ModelScaleProcess : public SqInitProcess{
	public:
		double &modelScale;
		ModelScaleProcess(double &modelScale) : modelScale(modelScale){}
		virtual void process(HSQUIRRELVM)const;
	};

	class HitboxProcess : public SqInitProcess{
	public:
		std::vector<hitbox> &hitboxes;
		HitboxProcess(std::vector<hitbox> &hitboxes) : hitboxes(hitboxes){}
		virtual void process(HSQUIRRELVM)const;
	};

	class HardPointProcess : public SqInitProcess{
	public:
		std::vector<hardpoint_static*> &hardpoints;
		HardPointProcess(std::vector<hardpoint_static*> &hardpoints) : hardpoints(hardpoints){}
		virtual void process(HSQUIRRELVM)const;
	};

	class DrawOverlayProcess : public SqInitProcess{
	public:
		GLuint &disp;
		DrawOverlayProcess(GLuint &disp) : disp(disp){}
		virtual void process(HSQUIRRELVM v)const;
	};

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

	class NavlightsProcess : public SqInitProcess{
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
	static const maneuve mymn;
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


/// Now this is interesting. If it is used properly, it does not use heap memory at all, so it
/// would be way faster than std::vector or something. In addition, the code is more readable too.
/// For example, expressions like sq_init("file", A <<= B <<= C) will chain the objects in the
/// stack.
///
/// The selection of the operator is arbitrary (nothing to do with bit shifting), but needs to be
/// right associative in order to process them in the order they appear in the argument
/// (in the above example, order of A, B, C), or this operator or sq_init() has to do recursive calls
/// somehow.
///
/// Also, += could be more readable, but it implies the left object accumulates the right
/// and the right could be deleted thereafter. It's not true; both object must be allocated
/// until sq_init() exits.
///
/// In association with std::streams, it could have << operator overloaded to do the same thing.
/// But the operator is left associative and we cannot simply assign neighboring object to next
/// unless we chain them in the reverse order (if the expression is like sq_init("file, A << B << C),
/// order of C, B, A). It would be not obvious to the user (of this class).
///
/// The appearance and usage of the operator remind me of Haskell Monads, but internal functioning is
/// nothing to do with them.
///
/// The argument, the returned value and the this pointer all must be const. That's because they
/// are temporary objects which are rvalues. In fact, we want to modify the content of this object to
/// construct a linked list, and it requires a mutable pointer (which is next).
///
/// The temporary objects resides in memory long enough to pass to the outermost function call.
/// That's guaranteed by the standard; temporary objects shall be alive within a full expression.
inline const Warpable::SqInitProcess &Warpable::SqInitProcess::operator<<=(const SqInitProcess &o)const{
	next = &o;
	return *this;
}
/*
SqInitProcess &SqInitProcess::operator<<(SqInitProcess &o){
	o.next = this;
	return o;
}
*/

/// Ordinary member function version of operator<<=(). It connects objects from right to left, which is the opposite
/// of the operator. The reason is that function calls in method chaining are usually left associative.
/// Of course you can call like <code>sq_init("file", A.chain(B.chain(C)))</code> to make it
/// right associative (sort of), but it would be a pain to keep the parentheses matched like LISP.
///
/// We just chose usual method chaining style (<code>sq_init("file", A.chain(B).chain(C))</code>), which is
/// more straightforward and maintainable. In exchange, passed objects are applied in the reverse order of
/// their appearance.
inline const Warpable::SqInitProcess &Warpable::SqInitProcess::chain(const SqInitProcess &o)const{
	o.next = this;
	return o;
}

inline Warpable::Navlight::Navlight() : pos(0,0,0), color(1,0,0,1), radius(0.01f), period(1.), phase(0.), pattern(Triangle), duty(0.1){
}

#endif
