#ifndef BULLET_H
#define BULLET_H
#include "entity.h"

#if 0
struct bullet_static{
	struct entity_static st;
	int (*const anim)(struct bullet *, warf_t *, struct tent3d_line_list *, double dt);
	void (*const draw)(struct bullet *, wardraw_t*);
	void (*const drawmodel)(struct bullet *, wardraw_t *); /* drawing of lit and depth checking model */
	void (*const postframe)(struct bullet *);
	struct entity *(*const get_homing_target)(struct bullet *); /* return NULL if none */
	int (*const is_bullet)(struct bullet*); /* if cause bleed when hit flesh */
};
#endif

class Bullet : public Entity{
public:
	typedef Entity st;

	double damage;
	float life;
	float runlength; // note that it's not age after creation
	Entity *owner;
	bool grav;

	Bullet(){}
	Bullet(Entity *owner, float life, double damage);
	virtual const char *idname()const;
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual double hitradius()const;
	virtual void anim(double dt);
	virtual void postframe();
	virtual void drawtra(wardraw_t *);
	virtual Entity *getOwner();
	virtual bool isTargettable()const;
protected:
	void bulletkill(int hitground, const struct contact_info *ci);
};

inline Bullet::Bullet(Entity *aowner, float alife, double adamage) : st(aowner->w), owner(aowner), damage(adamage), grav(false), life(alife), runlength(0){
	if(owner)
		race = owner->race;
	extern int bullet_shoots;
	bullet_shoots++;
}
/*
struct bullet *BulletNew(warf_t *w, entity_t *owner, double damage);
struct bullet *NormalBulletNew(warf_t *w, entity_t *owner, double damage);
struct bullet *ShotgunBulletNew(warf_t *w, entity_t *owner, double damage);
struct bullet *MortarHeadNew(warf_t *w, entity_t *owner, double damage);
struct bullet *TracerBulletNew(warf_t *w, entity_t *owner, double damage);
struct bullet *ExplosiveBulletNew(warf_t *w, entity_t *owner, double damage);
struct bullet *ShrapnelShellNew(warf_t *w, entity_t *owner, double damage);
struct bullet *APFSDSNew(warf_t *w, entity_t *owner, double damage);
struct bullet *BulletSwarmNew(warf_t *w, entity_t *owner, double damage, double rad, double vrad, int count);

int bullet_anim(struct bullet *, warf_t *, struct tent3d_line_list *, double dt);
void bullet_draw(struct bullet *, wardraw_t*);
void bullet_drawmodel(struct bullet *pb, wardraw_t *wd);
void bullet_postframe(struct bullet *pb);
entity_t *bullet_get_homing_target(struct bullet *pb);
*/
#endif
