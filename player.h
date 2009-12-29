#ifndef PLAYER_H
#define PLAYER_H
#include "serial.h"
#include <cstring>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>

#define FEETD 0.001

struct input_t;
struct tank;
struct astrobj;
struct coordsys;
class Entity;
class CoordSys;
struct teleport;

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

	Quatd getrot()const;
	Vec3d getpos()const;
	const char *classname()const; // returned string storage must be static
	void serialize(SerializeContext &sc);
	void unserialize(UnserializeContext &usc);
	void anim(double dt);
	void unlink(const Entity *);
	void freelook(const input_t &, double dt);
	void cockpitview(const input_t &, double dt);
	void tactical(const input_t &, double dt);
	static int cmd_mover(int argc, char *argv[], void *pv);
	static int cmd_coordsys(int argc, char *argv[], void *pv);
	static int cmd_position(int argc, char *argv[], void *pv);
	static int cmd_velocity(int argc, char *argv[], void *pv);
	static int cmd_teleport(int argc, char *argv[], void *pv);
	static teleport *findTeleport(const char *, int flags = ~0); // returns teleport node found
	static teleport *addTeleport(); // returns allocated uninitialized struct
	typedef unsigned teleport_iterator;
	static teleport_iterator beginTeleport();
	static teleport *getTeleport(teleport_iterator);
	static teleport_iterator endTeleport();
};



#endif
