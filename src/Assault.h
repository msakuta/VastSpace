/** \file
 * \brief Defines Assault class.
 */
#ifndef ASSAULT_H
#define ASSAULT_H
#include "Frigate.h"
#include "arms.h"
#include "Docker.h"
#include "VariantRegister.h"


/// Medium class ship with replaceable turrets.
class Assault : public Frigate{
public:
	typedef Frigate st;
	typedef ObservableList<ArmBase> TurretList;

	/// \brief A class register object that defines variant of Assault.
	///
	/// Note that this class is not singleton class, which means there could be multiple variants.
	typedef VariantRegister<Assault> AssaultVariantRegister;

protected:
	static ArmCtors armCtors; ///< Utility map to remember variant configurations.
	static std::list<AssaultVariantRegister*> assaultVariantRegisters; ///< Actuall buffer to hold reference to variant registrations.

	float engineHeat;
	WeakPtr<Assault> formPrev;
	TurretList turrets;

public:
	Assault(Game *game) : st(game){init();}
	Assault(WarField *w, const char *variant = NULL);
	~Assault();
	void init();
	static bool sqa_avr(HSQUIRRELVM);
	const char *idname()const;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	static const unsigned classid;
	static EntityRegister<Assault> entityRegister;
//	static AssaultGunnerRegister gunnerEntityRegister;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void anim(double);
	virtual void clientUpdate(double);
	virtual void draw(wardraw_t *);
	virtual void drawtra(wardraw_t *);
	virtual void drawOverlay(wardraw_t *);
	virtual double getHitRadius()const;
	virtual double getMaxHealth()const;
	virtual double maxshield()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int index);
	virtual int popupMenu(PopupMenu &list);
	virtual bool undock(Docker *);
	virtual bool command(EntityCommand *);
	virtual ManeuverParams &getManeuve()const;
	friend class GLWarms;
	static Entity *create(WarField *w, Builder *);
//	static const Builder::BuildStatic builds;
protected:
	bool buildBody();
	std::vector<hitbox> *getTraceHitBoxes()const;
	static double modelScale;
	static double hitRadius;
	static double defaultMass;
	static double maxHealthValue;
	static double maxShieldValue;
	static ManeuverParams mn;
	static std::vector<hardpoint_static*> hardpoints;
	static GLuint disp;
	static HitBoxList hitboxes;
	static std::vector<Navlight> navlights;
};

int cmd_armswindow(int argc, char *argv[], void *pv);

#endif
