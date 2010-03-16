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
	Player();
	~Player();
	Vec3d pos;
	Vec3d velo;
	Vec3d accel;
	Quatd rot;
	Vec3d cpos; // chase pos, used after chased object is destroyed to keep vision
	double rad;
	double flypower; // acceleration force
	double viewdist; // view distance from focused object
	double aviewdist; // actual viewdist approaching constantly to viewdist
	const CoordSys *cs;
	void (Player::*mover)(const input_t &inputs, double dt); /* virtual mover function */
	Entity *chase, *control, *selected, *lastchase;
	struct astrobj *sight;
	int chasecamera; /* multiple cameras can be mounted on a vehicle for having fun! */
	int trigger, detail, minimap;
	int mousex, mousey;
	int floortouch;
	int gear; /* acceleration gear in ghost mode */
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
	Quatd getrot()const;
	Vec3d getpos()const;
	const char *classname()const; // returned string storage must be static
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void unlink(const Entity *);
	void rotateLook(double dx, double dy);
	void freelook(const input_t &, double dt);
	void cockpitview(const input_t &, double dt);
	void tactical(const input_t &, double dt);
	void draw(Viewer *wd);
	void drawtra(Viewer *wd);
	void drawindics(Viewer *vw);
#ifdef _WIN32
	void mousemove(HWND hWnd, int deltax, int deltay, WPARAM wParam, LPARAM lParam);
#endif
	static int cmd_mover(int argc, char *argv[], void *pv);
	static int cmd_coordsys(int argc, char *argv[], void *pv);
	static int cmd_position(int argc, char *argv[], void *pv);
	static int cmd_velocity(int argc, char *argv[], void *pv);
	static int cmd_teleport(int argc, char *argv[], void *pv);
	static int cmd_moveorder(int argc, char *argv[], void *pv);
	static teleport *findTeleport(const char *, int flags = ~0); // returns teleport node found
	static teleport *addTeleport(); // returns allocated uninitialized struct
	typedef unsigned teleport_iterator;
	static teleport_iterator beginTeleport();
	static teleport *getTeleport(teleport_iterator);
	static teleport_iterator endTeleport();
};



#endif
