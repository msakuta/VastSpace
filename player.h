#ifndef PLAYER_H
#define PLAYER_H
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

class Player{
public:
	Player(){
		std::memset(this, 0, sizeof *this);
		rot[3] = 1.;
		fov = 1.;
		flypower = 1.;
		viewdist = 1.;
		mover = &Player::freelook;
	}
	Vec3d pos;
	Vec3d velo;
	Vec3d accel;
	Quatd rot;
	double rad;
	double flypower; // acceleration force
	double viewdist; // view distance from focused object
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
	void unlink(const Entity *);
	void freelook(const input_t &, double dt);
	void cockpitview(const input_t &, double dt);
	void tactical(const input_t &, double dt);
	static int cmd_mover(int argc, char *argv[], void *pv);
};



#endif
