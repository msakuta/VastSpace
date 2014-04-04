/** \file
 * \brief Definition of RStation class.
 */
#ifndef SPACEWAR_H
#define SPACEWAR_H
#include "Entity.h"
#include "judge.h"


#define VOLUME_PER_RU 100. /* 1 RU = 100m^3 */
#define RSTATION_MAX_RU 10000.
#define RSTATION_SCALE .1

/// \brief A stationary large structure that generate the resources over time.
class RStation : public Entity{
public:
	typedef Entity st;
	RStation(Game *game) : st(game){}
	RStation(WarField *w);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	static EntityRegister<RStation> entityRegister;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual double getMaxHealth()const;
	virtual double getHitRadius()const;
	virtual void anim(double dt);
	virtual void draw(wardraw_t *wd);
	virtual void drawtra(wardraw_t *wd);
	virtual int takedamage(double damage, int hitpart);
	virtual void cockpitview(Vec3d &pos, Quatd &rot, int seatid)const;
//static void rstation_control(struct entity*, warf_t*, const input_t *inputs, double dt);
//static int rstation_getrot(struct entity *pt, warf_t *w, double (*rot)[16]);
//static const char *rstation_idname(entity_t *pt){ return "rstation"; }
//static const char *rstation_classname(entity_t *pt){ return "Resource Station"; }
	virtual int popupMenu(PopupMenu &list);
//static void rstation_postframe(entity_t *);
	virtual int tracehit(const Vec3d &src, const Vec3d &dir, double rad, double dt, double *ret, Vec3d *retp, Vec3d *ret_normal);
	virtual Props props()const;

	double ru;
	double occupytime;
	int occupyrace;
protected:
	static HitBox rstation_hb[];
	static int numof_rstation_hb;
};

#endif
