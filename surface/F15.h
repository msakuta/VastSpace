/** \file
 * \brief Defines F-15 Eagle fighter class.
 */
#ifndef SURFACE_F15_H
#define SURFACE_F15_H

#include "Aerial.h"

/// \brief McDonnell Douglas F-15 Eagle
///
/// \see http://en.wikipedia.org/wiki/McDonnell_Douglas_F-15_Eagle
class F15 : public Aerial{
public:
	typedef Aerial st;
	static EntityRegister<F15> entityRegister;

	F15(Game *game);
	F15(WarField *aw);
	const char *idname()const override{return "f15";}
	const char *classname()const override{return "F15";}
	void anim(double dt)override;
	void draw(WarDraw *wd)override;
	void drawtra(WarDraw *wd)override;
	void drawHUD(WarDraw *)override;
	void drawCockpit(WarDraw *);
	void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const override;
	int F15::takedamage(double damage, int hitpart)override;
	double getHitRadius()const override{return 0.020;}
	bool isTargettable()const override{return true;}
	double getMaxHealth()const override{return maxHealthValue;}

	static double getModelScale(){return modelScale;}

protected:
	Tefpol *pf, *vapor[2];

	static Model *model;
	static double modelScale;
	static double defaultMass; ///< Dry mass?
	static double maxHealthValue;
	static WingList wings0;
	static HitBoxList hitboxes;

	HitBoxList &getHitBoxes()const override{return hitboxes;}
	void shoot(double dt)override;

	void init();
};

#endif
