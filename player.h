#ifndef PLAYER_H
#define PLAYER_H
#include "serial.h"
#include <cstring>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/mat4.h>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif
#include <squirrel.h>

#define FEETD 0.001

struct input_t;
struct tank;
struct astrobj;
struct coordsys;
class Entity;
class CoordSys;
class Viewer;
struct teleport;
struct war_draw_data;


class Player : public Serializable{
public:
	class mover_t{
	public:
		Player &pl;
		mover_t(Player &a) : pl(a){}
		virtual void operator ()(const input_t &inputs, double dt);
		virtual Vec3d getpos()const;
		virtual void setpos(const Vec3d &apos){pl.pos = apos;}
		virtual Vec3d getvelo()const{return pl.velo;}
		virtual void setvelo(const Vec3d &apos){pl.pos = apos;}
		virtual Quatd getrot()const;
		virtual void setrot(const Quatd &arot){pl.rot = arot;}
		virtual void rotateLook(double dx, double dy);
	protected:
		Vec3d &refvelo(){return pl.velo;}
	};
	class FreelookMover;
protected:
	Vec3d getrawpos(mover_t*)const;
	Quatd getrawrot(mover_t*)const;
public:
	Player();
	~Player();
protected:
	Vec3d pos;
	Vec3d velo;
	Vec3d accel;
	Quatd rot;
public:
	Vec3d cpos; // chase pos, used after chased object is destroyed to keep vision
	double rad;
//	double flypower; // acceleration force
	double viewdist; // view distance from focused object
	double aviewdist; // actual viewdist approaching constantly to viewdist
	const CoordSys *cs;
	mover_t *mover; /* virtual mover function */
	mover_t *nextmover; // next mover function, interpolate with mover at factor of blendmover to smoothly switch modes
	float blendmover;
	Entity *chase, *control, *selected, *lastchase;
	struct astrobj *sight;
	int chasecamera; /* multiple cameras can be mounted on a vehicle for having fun! */
	int trigger, detail, minimap;
	int mousex, mousey;
	int floortouch;
	int race;
	double fov;
	double gametime;
	double velolen; /* trivial; performance keeper */
	double height; /* trivial; indicates air pressure surrounding the player */
	std::set<const Entity*> chases; // Chased group of Entities. viewing volume shows all of them.
	bool moveorder; // Issueing moving order
	bool move_lockz; /* 3-D movement order */
	double move_z;
	double move_t; // ray trace factor
	Vec3d move_org;
	Vec3d move_src;
	Vec3d move_hitpos;
	Vec3d move_viewpos;
	Mat4d move_rot;
	Mat4d move_model;
	Mat4d move_proj;
	Mat4d move_trans;

	void free(); // Frees internal memories but keep the object memory
	Vec3d getpos()const;
	void setpos(const Vec3d &apos){mover->setpos(apos);}
	Vec3d getvelo()const{return mover->getvelo();}
	void setvelo(const Vec3d &apos){mover->setvelo(apos);}
	Quatd getrot()const;
	void setrot(const Quatd &arot){mover->setrot(arot);}
	const CoordSys *getcs()const{return cs;}
	const char *classname()const; // returned string storage must be static
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void transit_cs(CoordSys *cs); // Explicitly change current CoordSys, keeping position, velocity and rotation.
	void unlink(const Entity *);
	void rotateLook(double dx, double dy);
	FreelookMover *const freelook;
	mover_t *const cockpitview;
	mover_t *const tactical;
	void draw(Viewer *wd);
	void drawtra(Viewer *wd);
	void drawindics(Viewer *vw);
#ifdef _WIN32
	void mousemove(HWND hWnd, int deltax, int deltay, WPARAM wParam, LPARAM lParam);
#endif
	static float camera_mode_switch_time;
	static void cmdInit(Player &pl);
	static int cmd_mover(int argc, char *argv[], void *pv);
	static int cmd_teleport(int argc, char *argv[], void *pv);
	static int cmd_moveorder(int argc, char *argv[], void *pv);
	static teleport *findTeleport(const char *, int flags = ~0); // returns teleport node found
	static teleport *addTeleport(); // returns allocated uninitialized struct
	typedef unsigned teleport_iterator;
	static teleport_iterator beginTeleport();
	static teleport *getTeleport(teleport_iterator);
	static teleport_iterator endTeleport();

	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_set(HSQUIRRELVM v);

	static SQInteger sqf_getpos(HSQUIRRELVM v);
	static SQInteger sqf_setpos(HSQUIRRELVM v);

	static SQInteger sqf_getrot(HSQUIRRELVM v);
	static SQInteger sqf_setrot(HSQUIRRELVM v);

private:
//	int gear; /* acceleration gear in ghost mode */
};

typedef class Player::FreelookMover : public Player::mover_t{
public:
	typedef Player::mover_t st;
	double flypower;
	int gear;
	Vec3d pos;
	FreelookMover(Player &a);
	void operator ()(const input_t &inputs, double dt);
	Quatd getrot()const;
	Vec3d getpos()const;
	void setGear(int g);
	int getGear()const;
} FreelookMover;



inline int FreelookMover::getGear()const{return gear;}


#endif
