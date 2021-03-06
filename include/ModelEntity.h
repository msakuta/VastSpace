/** \file
 * \brief Definition of ModelEntity class.
 */
#ifndef MODELENTITY_H
#define MODELENTITY_H

#include "Entity.h"
#include "Game.h"
#include "SqInitProcess.h"
#include "judge.h"
#include <btBulletDynamicsCommon.h> // btCompoundShape
#ifdef DEDICATED
typedef unsigned GLuint;
#else
#include <GL/gl.h>
#endif

class EXPORT HitBoxList : public std::vector<HitBox>{
};

/// \brief An Entity having mesh model (which is defined by class Model) and rigid body.
///
/// This extra layer of inheritance merely introduces static protected functions that helps
/// to load values from configuration script files.
class EXPORT ModelEntity : public Entity{
public:
	typedef Entity st;

	ModelEntity(Game *game) : st(game){}
	ModelEntity(WarField *w) : st(w){}

protected:
	void addRigidBody(WarSpace *)override;
	virtual bool buildBody();
	virtual short bbodyGroup()const;
	virtual short bbodyMask()const;

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

	class EXPORT HitboxProcess : public SqInitProcess{
	public:
		HitBoxList &hitboxes;
		HitboxProcess(HitBoxList &hitboxes) : hitboxes(hitboxes){}
		virtual void process(HSQUIRRELVM)const;
	};

public:
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

	/// \brief List of Navlight objects symbolized
	typedef std::vector<Navlight> NavlightList;

	class EXPORT NavlightsProcess : public SqInitProcess{
	public:
		NavlightList &navlights;
		NavlightsProcess(NavlightList &navlights) : navlights(navlights){}
		virtual void process(HSQUIRRELVM v)const;
	};

	/// \brief Helper function to draw navigation lights.
	/// \param wd The WarDraw object.
	/// \param navlights List of navigation lights.
	/// \param transmat The transformation for the lights. Can be NULL to specify
	///        the default transformation, i.e. return value of transform().
	void drawNavlights(WarDraw *wd, const NavlightList &navlights, const Mat4d *transmat = NULL);

	/// \brief An utility function that constructs a shape by a HitBoxList.
	/// \param hitboxes The input hitboxes, probably loaded from a initialization script.
	/// \param shapeStorage The static storage for a class to store shape object for reuse.
	///        Entities have distinct shapes, but the same Entity class instances usually share
	///        the shape.  In this case, pass a static shape object to this argument.
	bool buildBodyByHitboxes(const HitBoxList &hitboxes, btCompoundShape *&shapeStorage);
};



//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------

inline ModelEntity::Navlight::Navlight() : pos(0,0,0), color(1,0,0,1), radius(0.01f), period(1.), phase(0.), pattern(Triangle), duty(0.1f){
}


#endif
