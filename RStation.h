#ifndef SPACEWAR_H
#define SPACEWAR_H
#include "entity.h"


#define VOLUME_PER_RU 100. /* 1 RU = 100m^3 */

class RStation : public Entity{
public:
	typedef Entity st;
	RStation(){}
	RStation(WarField *w);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual const char *dispname()const;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual bool isTargettable()const;
	virtual bool isSelectable()const;
	virtual double maxhealth()const;
	virtual double hitradius()const;
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
};

#endif
