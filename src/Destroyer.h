/** \file
 * \brief Definition of Destroyer class.
 */
#ifndef DESTROYER_H
#define DESTROYER_H
#include "Warpable.h"
#include "arms.h"
#include "VariantRegister.h"

/// \brief A Destroyer class space warship equipped with large turrets and thich armor.
class Destroyer : public Warpable{
protected:
	typedef ObservableList<ArmBase> TurretList;
	float engineHeat;
	float buildPhase;
	TurretList turrets;
	bool clientDead; ///< A flag indicating the death effects are performed in the client.
	static std::vector<hardpoint_static*> hardpoints;
	static ArmCtors armCtors;
	static std::list<VariantRegister<Destroyer>*> variantRegisters;
	static HitBoxList hitboxes;
	static double modelScale;
	static double defaultMass;
	static double maxHealthValue;
	static ManeuverParams maneuverParams;
	static GLuint disp;
	static std::vector<Navlight> navlights;
public:
	typedef Warpable st;
	Destroyer(Game *game) : st(game), clientDead(false){init();}
	Destroyer(WarField *w, const SQChar *variant = "Standard");
	~Destroyer();
	static const unsigned classid;
	static EntityRegister<Destroyer> entityRegister;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double getHitRadius()const;
	virtual void anim(double dt);
	virtual void clientUpdate(double dt);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	virtual void draw(wardraw_t *wd);
	virtual void drawtra(wardraw_t *wd);
	virtual void drawOverlay(wardraw_t *);
	virtual int takedamage(double damage, int hitpart);
	virtual double maxhealth()const;
	virtual int armsCount()const;
	virtual ArmBase *armsGet(int index);
	virtual int popupMenu(PopupMenu &list);
	virtual bool command(EntityCommand *com);
	virtual double maxenergy()const;
	const ManeuverParams &getManeuve()const;
	virtual double warpCostFactor()const;

	static bool static_init(HSQUIRRELVM);
protected:
	bool buildBody();
	std::vector<hitbox> *getTraceHitBoxes()const;
	virtual void init();
	void deathEffects();
	friend class WireDestroyer;
};

class WireDestroyer : public Warpable{
protected:
	double wirephase;
	double wireomega;
	double wirelength;
	static struct hitbox hitboxes[];
	static const int nhitboxes;
public:
	typedef Warpable st;
	WireDestroyer(Game *game) : st(game){}
	WireDestroyer(WarField *w);
	static const unsigned classid;
	static EntityRegister<WireDestroyer> entityRegister;
	virtual const char *classname()const;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double getHitRadius()const;
	virtual double maxhealth()const;
	virtual double maxenergy()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *wd);
	virtual void drawtra(wardraw_t *wd);
	virtual int tracehit(const Vec3d &start, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *retn);
	virtual void cockpitView(Vec3d &pos, Quatd &rot, int seatid)const;
	const ManeuverParams &getManeuve()const;
};

#endif
