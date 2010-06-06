#ifndef WAR_H
#define WAR_H
#include "serial.h"
#include "tent3d.h"
extern "C"{
#include <clib/colseq/color.h>
#include <clib/avec3.h>
#include <clib/amat3.h>
#include <clib/suf/suf.h>
#include <clib/rseq.h>
}
#include <cpplib/RandomSequence.h>
#include <cpplib/gl/cullplus.h>

/* bitmask for buttons */
#define PL_W 0x01
#define PL_S 0x02
#define PL_A 0x04
#define PL_D 0x08
#define PL_Q 0x10
#define PL_Z 0x20
#define PL_E 0x8000
#define PL_8 0x40
#define PL_2 0x80
#define PL_4 0x100
#define PL_6 0x200
#define PL_7 0x400
#define PL_9 0x800
#define PL_SPACE 0x1000
#define PL_ENTER 0x2000
#define PL_TAB 0x4000
#define PL_LCLICK 0x10000 /* left mouse button */
#define PL_RCLICK 0x20000
#define PL_B 0x40000
#define PL_G 0x80000
#define PL_R   0x100000
#define PL_MWU 0x200000 /* mouse wheel up */
#define PL_MWD 0x400000 /* mouse wheel down */
#define PL_F   0x800000
#define PL_SHIFT 0x1000000
#define PL_CTRL  0x2000000

class Player;
class Viewer;
class CoordSys;
class Entity;
class Bullet;

/* all player or AI's input should be expressed in this structure. */
struct input_t{
	unsigned press; /* bitfield for buttons being pressed */
	unsigned change; /* and changing state */
	int start; /* if the controlling is beggining */
	int end; /* if the controlling is terminating */
	double analog[4]; /* analog inputs, first and second elements are probably mouse movements. */
	input_t() : press(0), change(0), start(0), end(0){
		analog[0] = analog[1] = analog[2] = analog[3] = 0;
	}
};

typedef struct warmap warmap_t;
typedef struct warmapdecal_s warmapdecal_t;

extern suf_t *CacheSUF(const char *fname);

typedef struct suf_model{
	suf_t *suf;
	double trans[16];
	double bs[3]; /* trivial; bounding sphere center */
	double rad; /* trivial; bounding sphere radius */
	int valid; /* validity of bounding sphere */
} sufmodel_t;

typedef struct war_race{
	double money;
	double moneygain;
	int kills, deaths;
	int techlevel;
	struct tank *homebase;
} race_t;

struct contact_info{
	Vec3d normal;
	Vec3d velo; /* base velocity of colliding object */
	double depth; /* penetration depth */
};
struct otnt;

struct war_draw_data;
struct tent3d_line_list;

struct war_field_static{
	void (*const accel)(struct war_field *, avec3_t *dst, const avec3_t *srcpos, const avec3_t *srcvelo);
	double (*const atmospheric_pressure)(struct war_field *, const avec3_t *pos);
	double (*const sonic_speed)(struct war_field *, const avec3_t *pos); /* it can change in various planets */
	int (*const spherehit)(struct war_field*, const double (*pos)[3], double rad, struct contact_info*);
	int (*const orientation)(struct war_field*, amat3_t *dst, const avec3_t *pos);
	double (*const nearplane)(const struct war_field*); /* distance at objects begin to show */
	double (*const farplane)(const struct war_field*); /* distance at objects begin to hide (not if farmap) */
	int (*const inwater)(const struct war_field*, const avec3_t *pos); /* TODO: return viscosity? (for non-water oceans) */
};

class WarField;
class Entlist{
public:
	virtual Entity *addent(Entity *) = 0;
	virtual Entity *entlist() = 0;
	virtual Player *getPlayer() = 0;
	virtual operator WarField&() = 0;
};

class WarSpace;
class Docker;

class WarField : public Serializable, public Entlist{
public:
	WarField();
	WarField(CoordSys *cs);
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
	virtual void dive(SerializeContext &, void (Serializable::*)(SerializeContext &));
	virtual void anim(double dt);
	virtual void postframe(), endframe(); // postframe gives an opportunity to clear pointers to objects being destroyed, while endframe actually destruct them.
	virtual void draw(struct war_draw_data *);
	virtual void drawtra(struct war_draw_data *);
	virtual bool pointhit(const Vec3d &pos, const Vec3d &velo, double dt, struct contact_info*)const;
	virtual Vec3d accel(const Vec3d &srcpos, const Vec3d &srcvelo)const;
	virtual double war_time()const;
	virtual struct tent3d_line_list *getTeline3d();
	virtual struct tent3d_fpol_list *getTefpol3d();
	Entity *addent(Entity *);
	Entity *entlist(){return el;}
	Player *getPlayer();
	operator WarField&(){return *this;}
	virtual operator WarSpace*();
	virtual operator Docker*();
	template<Entity *WarField::*list> int countEnts()const;
	int countBullets()const;

	CoordSys *cs; // redundant pointer to indicate belonging coordinate system
	Player *pl;
	Entity *el; /* entity list */
	Entity *bl; /* bullet list */
	RandomSequence rs;
	double realtime;
};

class btDiscreteDynamicsWorld;

class WarSpace : public WarField{
	void init();
public:
	typedef WarField st;
	WarSpace();
	WarSpace(CoordSys *cs);
	virtual const char *classname()const;
	static const unsigned classid;
	virtual void serialize(SerializeContext &sc);
	virtual void unserialize(UnserializeContext &sc);
//	virtual void dive(SerializeContext &, void (Serializable::*)(SerializeContext &));
	virtual void anim(double dt);
	virtual void endframe();
	virtual void draw(struct war_draw_data *);
	virtual void drawtra(struct war_draw_data *);
	virtual struct tent3d_line_list *getTeline3d();
	virtual struct tent3d_fpol_list *getTefpol3d();
	virtual operator WarSpace*();

	struct tent3d_line_list *tell, *gibs;
	struct tent3d_fpol_list *tepl;
	int effects; /* trivial; telines or tefpols generated by this frame: to control performance */
	double soundtime;
	otnt *ot, *otroot;
	int ots, oti;

	btDiscreteDynamicsWorld *bdw;

	static int g_otdrawflags;
};

typedef struct war_draw_data{
//	unsigned listLight; /* OpenGL list name to switch on a light to draw solid faces */
//	void (*light_on)(void); /* processes to turn */
//	void (*light_off)(void); /* the light on and off. */
//	COLOR32 ambient; /* environmental color; non light-emitting objects dyed by this */
//	unsigned char hudcolor[4];
//	int irot_init; /* inverse rotation */
//	double (*rot)[16]; /* rotation matrix */
//	double irot[16];
//	double view[3]; /* view position */
//	double viewdir[3]; /* unit vector pointing view direction */
//	double fov; /* field of view */
//	GLcull *pgc;
	Viewer *vw;
//	double light[3]; /* light direction */
//	double gametime;
	double maprange;
	int lightdraws;
	WarField *w;
} wardraw_t;


template<Entity *WarField::*list> inline int WarField::countEnts()const{
	int ret = 0;
	for(Entity *p = this->*list; p; p = p->next)
		ret++;
	return ret;
}


#endif
