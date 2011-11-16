#ifndef PLAYER_H
#define PLAYER_H
#include "serial.h"
#include "Entity.h"
//#include <cstring>
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#include <cpplib/mat4.h>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif
#include <squirrel.h>

/** \file
 * \brief Definition of the Player.
 *
 * The player is modeled as a class object in C++.
 * This file defines the Player class and camera related classes.
 */

#define FEETD 0.001

struct input_t;
struct tank;
struct astrobj;
struct coordsys;
class Entity;
class CoordSys;
class Viewer;
struct teleport;
class GLWstateButton;

/** \brief The Player in front of display.
 *
 * Conceptually, the camera and the player is separate entity, but they heavily depend on each other.
 */
class EXPORT Player : public EntityController{
public:

	/// Base class for camera controller classes.
	///
	/// It defines how the camera reacts to user inputs. There're multiple ways to navigate the camera
	/// in 3-D space, so we virtualize it with this class.
	class CameraController{
	public:
		Player &pl;
		CameraController(Player &a) : pl(a){}
		virtual void operator ()(const input_t &inputs, double dt);
		virtual Vec3d getpos()const;
		virtual void setpos(const Vec3d &apos){pl.pos = apos;}
		virtual Vec3d getvelo()const{return pl.velo;}
		virtual void setvelo(const Vec3d &avelo){pl.velo = avelo;}
		virtual Quatd getrot()const;
		virtual void setrot(const Quatd &arot){pl.rot = arot;}
		virtual void rotateLook(double dx, double dy);
	protected:
		Vec3d &refvelo(){return pl.velo;}
	};

	class FreelookMover;
protected:
	virtual bool control(Entity *, double dt);
	Vec3d getrawpos(CameraController*)const;
	Quatd getrawrot(CameraController*)const;
public:
	Player();
	~Player();
protected:
	Vec3d pos;
	Vec3d velo;
	Vec3d accel;
	Quatd rot;
public:
	Vec3d cpos; ///< chase pos, used after chased object is destroyed to keep vision
	double rad;
//	double flypower; // acceleration force
	double viewdist; ///< view distance from focused object
	double aviewdist; ///< actual viewdist approaching constantly to viewdist
	const CoordSys *cs;
	CameraController *mover; ///< virtual mover function
	CameraController *nextmover; ///< next mover function, interpolate with mover at factor of blendmover to smoothly switch modes
	float blendmover; ///< Blending factor of mover and nextmover.
	Entity *chase, *controlled, *selected, *lastchase; ///< Various entity lists
	int chasecamera; ///< Camera ID of chased object. Multiple cameras can be mounted on a vehicle for having fun!
	int detail;
	int mousex, mousey;
	int race;
	bool r_move_path;
	bool r_attack_path;
	bool r_overlay;
	double fov; ///< Field of view value, in cosine angle
	double gametime; ///< global time
	double velolen; ///< trivial; performance keeper
	double height; ///< trivial; indicates air pressure surrounding the player
	std::set<const Entity*> chases; ///< Chased group of Entities. viewing volume shows all of them.
	int attackorder; ///< Issueing attacking order
	int forceattackorder;
	bool moveorder; ///< Issueing moving order
	bool move_lockz; ///< 3-D movement order
	double move_z;
	double move_t; ///< ray trace factor
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
	CameraController *const cockpitview;
	CameraController *const tactical;
	void draw(Viewer *wd);
	void drawtra(Viewer *wd);
	void drawindics(Viewer *vw);
#ifdef _WIN32
	void mousemove(HWND hWnd, int deltax, int deltay, WPARAM wParam, LPARAM lParam);
#endif
	/// Quit controlling an Entity.
	void uncontrol(){
		if(controlled && controlled->controller == this)
			controlled->controller = NULL;
		controlled = NULL;
	}
	Quatd orientation()const;

	static float camera_mode_switch_time;
	static int g_overlay; // Overlay display level
	static void cmdInit(Player &pl);
	static int cmd_mover(int argc, char *argv[], void *pv);
	static int cmd_teleport(int argc, char *argv[], void *pv);
	static int cmd_moveorder(int argc, char *argv[], void *pv);
	static int cmd_control(int argc, char *argv[], void *pv); ///< Switch control of selected Entity
	static teleport *findTeleport(const char *, int flags = ~0); // returns teleport node found
	static teleport *addTeleport(); // returns allocated uninitialized struct
	typedef unsigned teleport_iterator;
	static teleport_iterator beginTeleport();
	static teleport *getTeleport(teleport_iterator);
	static teleport_iterator endTeleport();

	/// Define class Player for Squirrel.
	static void sq_define(HSQUIRRELVM v);

	/// Creates a button to toggle control of selected Entity.
	static GLWstateButton *newControlButton(Player &pl, const char *filename, const char *filename2, const char *tips);

	/// Creates a button to toggle move ordering mode.
	static GLWstateButton *newMoveOrderButton(Player &pl, const char *filename, const char *filename2, const char *tips);

private:
	static SQInteger sqf_get(HSQUIRRELVM v);
	static SQInteger sqf_set(HSQUIRRELVM v);

	static SQInteger sqf_getpos(HSQUIRRELVM v);
	static SQInteger sqf_setpos(HSQUIRRELVM v);

	static SQInteger sqf_getrot(HSQUIRRELVM v);
	static SQInteger sqf_setrot(HSQUIRRELVM v);

	static SQInteger sqf_setmover(HSQUIRRELVM v);
	static SQInteger sqf_getmover(HSQUIRRELVM v);
//	int gear; /* acceleration gear in ghost mode */
};

typedef class Player::FreelookMover : public Player::CameraController{
public:
	typedef Player::CameraController st;
	double flypower;
	int gear;
	Vec3d pos;
	FreelookMover(Player &a);
	void operator ()(const input_t &inputs, double dt);
	Quatd getrot()const;
	void setpos(const Vec3d &apos);
	Vec3d getpos()const;
	void setGear(int g);
	int getGear()const;
} FreelookMover;



inline int FreelookMover::getGear()const{return gear;}

extern void capture_mouse();

#endif